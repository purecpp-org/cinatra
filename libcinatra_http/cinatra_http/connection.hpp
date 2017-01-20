
#pragma once

#include <cinatra_http/response.h>
#include <cinatra_http/request.h>
#include <cinatra_http/utils.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <vector>

#include <cassert>

namespace cinatra
{
	using request_handler_t = std::function<void(const request& req, response& rep)>;

	template <typename socket_type>
	class connection
		: public std::enable_shared_from_this<connection<socket_type>>,
		private boost::noncopyable
	{
	public:
		explicit connection(boost::asio::io_service& io_service, request_handler_t& handler)
			: socket_(io_service), request_handler_(handler), deadline_(io_service)
		{
			request_.raw_request().size = 0;
		}

		explicit connection(boost::asio::io_service& io_service, request_handler_t& handler, boost::asio::ssl::context& ctx)
			: socket_(io_service, ctx), request_handler_(handler), deadline_(io_service)
		{
			request_.raw_request().size = 0;
		}

		socket_type&  socket()
		{
			return socket_;
		}

		void start()
		{
			std::weak_ptr<connection<socket_type>> weak_self = this->shared_from_this();
			reply_.set_get_connection_func([weak_self, this](bool kill_timer)
			{
				auto self = weak_self.lock();
				if (!self)
				{
					return std::shared_ptr<response::connection>();
				}

				if (kill_timer)
				{
					boost::system::error_code ec;
					deadline_.cancel(ec);
				}
				return std::make_shared<response::connection>(reply_,
					[self, this](const void* data, std::size_t size, response::handler_ec_size_t handler) {delay_write(data, size, std::move(handler));},
					[self, this](std::vector<boost::asio::const_buffer> const& buffers, response::handler_ec_size_t handler) {delay_write(buffers, std::move(handler));},

					[self, this](void* data, std::size_t size, response::handler_ec_size_t handler) {delay_read(data, size, handler);},
					[self, this](void* data, std::size_t size, response::handler_ec_size_t handler) {delay_read_some(data, size, handler);},
					[self, this](response::handler_strref_intptr_t handler) {delay_read_chunk(handler);},

					[self, this](response::handler_ec_t) {shutdown(socket_);},
					[self, this]() {close();},

					[self, this]() {return !socket().lowest_layer().is_open();},

					[self, this]()
					{
						if (reply_.body_type() != response::none)
						{
							do_write();
							return;
						}

						reply_.reset();

						if (!keep_alive_)
						{
							shutdown(socket_);
							return;
						}

						request_.raw_request().size = 0;
						do_read();
					}
				);
			});


			do_read();
		}

		void close()
		{
			do_close(socket_);
		}

		void reset_timer(int seconds = 60)
		{
			deadline_.expires_from_now(boost::posix_time::seconds(seconds));	//TODO:超时时间改为可配置
			std::weak_ptr<connection<socket_type>> weak_self = this->shared_from_this();

			deadline_.async_wait([weak_self](boost::system::error_code const& ec)
			{
				auto self = weak_self.lock();

				if (!self || ec)
				{
					return;
				}

				self->close();
			});
		}

	private:
		void do_close(boost::asio::ip::tcp::socket const&)
		{
			boost::system::error_code ec;
			socket_.close(ec);
		}
		void do_close(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> const&)
		{
			boost::system::error_code ec;
			socket_.lowest_layer().close(ec);
		}

		void do_read()
		{
			reset_timer();
			auto& buf = request_.raw_request();
			if (buf.remain_size() < 4096)
			{
				request_.increase_buffer(8192);
			}
			socket_.async_read_some(boost::asio::buffer(buf.curr_ptr(), buf.remain_size()),
				boost::bind(&connection::handle_read, this->shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		void do_read_body()
		{
			reset_timer();
			std::size_t req_len = request_.header_size() + request_.body_len();
			auto& buf = request_.raw_request();
			if (buf.max_size < req_len)
			{
				request_.increase_buffer(req_len - request_.raw_request().max_size);
			}

			auto self = this->shared_from_this();
			boost::asio::async_read(socket_, boost::asio::buffer(buf.curr_ptr(), req_len - buf.size),
				[self, this](boost::system::error_code ec, std::size_t length)
			{
				if (ec)
				{
					return;
				}

				request_.raw_request().size += length;

				auto content_type = request_.get_header("Content-Type", 12);
				if (!content_type.empty())
				{
					if (content_type.find("application/x-www-form-urlencoded") != boost::string_ref::npos)
					{
						request_.parse_form_urlencoded();	//TODO:解析失败?
					}
					else if (content_type.find("multipart/form-data") != boost::string_ref::npos)
					{
						request_.parse_form_multipart();	//TODO:解析失败?
					}

				}
				do_request();
			});
		}

		void do_write()
		{
			reset_timer();

			check_keep_alive();

			assert(reply_.headers_num("Content-Length", 14) == 1);

			std::vector<boost::asio::const_buffer> buffers;
			write_finished_ = reply_.to_buffers(buffers);
			if (buffers.empty())
			{
				handle_write(boost::system::error_code{});
				return;
			}

			boost::asio::async_write(socket_, buffers,
				boost::bind(&connection::handle_write, this->shared_from_this(),
					boost::asio::placeholders::error));
		}

		void do_request()
		{
			if (request_handler_)
			{
				request_handler_(request_, reply_);
			}
			else
			{
				reply_ = response::stock_reply(response::not_found);
			}

			if (reply_.is_delay())
			{
				return;
			}
			do_write();
		}

		void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred)
		{
			if (e)
			{
				if (e == boost::asio::error::eof)
				{
					shutdown(socket_);
				}
				return;
			}

			auto& buf = request_.raw_request();
			auto last_len = buf.size;
			buf.size += bytes_transferred;

			if (buf.size >= 2 * 1024 * 1024)
			{
				// 请求过大,断开链接
				return;
			}

			int ret = request_.parse_header(last_len);

			if (ret == -1)
			{
				//TODO: 判断是否可以升级到http2
				reply_ = response::stock_reply(response::bad_request);
				do_write();
				return;
			}

			if (ret == -2)
			{
				do_read();
				return;
			}


			if (request_.header_size() + request_.body_len() >= 2 * 1024 * 1024)
			{
				return;
			}

			if (request_.body_len() != 0)
			{
				do_read_body();
				return;
			}

			do_request();
		}

		void handle_write(const boost::system::error_code& e)
		{
			if (e)
			{
				return;
			}

			if (write_finished_)
			{
				reply_.reset();

				if (!keep_alive_)
				{
					shutdown(socket_);
					return;
				}

				request_.raw_request().size = 0;
				do_read();
				return;
			}
			do_write();
		}

		void check_keep_alive()
		{
			auto rep_conn_hdr = reply_.get_header("connection", 10);
			if (rep_conn_hdr.empty())
			{
				auto req_conn_hdr = request_.get_header("connection", 10);
				if (request_.is_http1_1())
				{
					// HTTP1.1
					//头部中没有包含connection字段
					//或者头部中包含了connection字段但是值不为close
					//这种情况是长连接
					//keep_alive_ = conn_hdr.empty() || !boost::iequals(conn_hdr, "close");
					keep_alive_ = req_conn_hdr.empty() || !iequal(req_conn_hdr.data(), req_conn_hdr.size(), "close", 5);
				}
				else
				{
					//HTTP1.0或其他(0.9 or ?)
					//头部包含connection,并且connection字段值为keep-alive
					//这种情况下是长连接
					//keep_alive_ = !conn_hdr.empty() && boost::iequals(conn_hdr, "keep-alive");
					keep_alive_ = !req_conn_hdr.empty() && iequal(req_conn_hdr.data(), req_conn_hdr.size(), "keep-alive", 10);
				}

				if (keep_alive_)
				{
					reply_.add_header("Connection", "keep-alive");
				}
				else
				{
					reply_.add_header("Connection", "close");
				}

				return;
			}

			keep_alive_ = iequal(rep_conn_hdr.data(), rep_conn_hdr.size(), "keep-alive", 10);
		}

		//TODO: shutdown之后继续read,读取到eof再关闭链接
		void shutdown(boost::asio::ip::tcp::socket const&)
		{
			boost::system::error_code ignored_ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ignored_ec);
		}
		void shutdown(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> const&s)
		{
			//s.async_shutdown()
		}


	private:
		void delay_write(const void* data, std::size_t size, response::handler_ec_size_t handler)
		{
			reset_timer();
			if (!reply_.header_buffer_wroted())
			{
				check_keep_alive();
				assert(reply_.body_type() == response::none);
				std::vector<boost::asio::const_buffer> buffers;
				auto finished = reply_.to_buffers(buffers);
				assert(finished);
				auto self = this->shared_from_this();
				boost::asio::async_write(socket_, buffers,
					[self, this, data, size, handler](const boost::system::error_code& ec, std::size_t /*length*/)
				{
					if (ec)
					{
						handler(ec, 0);
						return;
					}

					delay_write(data, size, std::move(handler));
				});
				return;
			}

			boost::asio::async_write(socket_, boost::asio::buffer(data, size), handler);
		}

		void delay_write(std::vector<boost::asio::const_buffer> const& buffers, response::handler_ec_size_t handler)
		{
			reset_timer();
			// TODO:合并
			if (!reply_.header_buffer_wroted())
			{
				check_keep_alive();
				assert(reply_.body_type() == response::none);
				std::vector<boost::asio::const_buffer> buffers;
				auto finished = reply_.to_buffers(buffers);
				assert(finished);
				auto self = this->shared_from_this();
				boost::asio::async_write(socket_, buffers,
					[self, this, buffers, handler](const boost::system::error_code& ec, std::size_t /*length*/)
				{
					if (ec)
					{
						handler(ec, 0);
						return;
					}

					delay_write(buffers, std::move(handler));
				});
				return;
			}

			boost::asio::async_write(socket_, buffers, handler);
		}

		void delay_read(void* data, std::size_t size, response::handler_ec_size_t handler)
		{
			reset_timer();
			boost::asio::async_read(socket_, boost::asio::buffer(data, size), handler);
		}

		void delay_read_some(void* data, std::size_t size, response::handler_ec_size_t handler)
		{
			reset_timer();
			socket_.async_read_some(boost::asio::buffer(data, size), handler);
		}

		void delay_read_chunk(response::handler_strref_intptr_t handler)
		{
			auto& buf = request_.raw_request();
			if (buf.size > request_.header_size())
			{
				std::size_t remain_size = buf.size - request_.header_size();
				auto chunk_start = buf.buffer + request_.header_size();
				auto ret = phr_decode_chunked(&chunked_dec_, chunk_start, &remain_size);
				handler(boost::string_ref(chunk_start, remain_size), ret);
				buf.size = request_.header_size();
				return;
			}


			chunked_buf_.resize(2 * 1024 * 1024);
			delay_read_some(&chunked_buf_[0], chunked_buf_.size(),
				[this, handler](const boost::system::error_code& ec, std::size_t length)
			{
				if (ec)
				{
					handler(boost::string_ref(), -1);
					return;
				}

				auto ret = phr_decode_chunked(&chunked_dec_, &chunked_buf_[0], &length);
				handler(boost::string_ref(chunked_buf_.data(), length), ret);
			});

		}


	private:
		socket_type socket_;

		request_handler_t& request_handler_;

		request request_;

		std::string chunked_buf_;
		phr_chunked_decoder chunked_dec_ = { 0 };

		bool write_finished_;
		response reply_;

		bool keep_alive_ = false;

		boost::asio::deadline_timer deadline_;
	};
}
