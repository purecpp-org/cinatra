#include <cinatra_http/websocket.h>
#include <cinatra_http/utils.h>

#include <boost/smart_ptr.hpp>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

namespace cinatra
{
	namespace websocket
	{

		websocket_connection::websocket_connection(std::shared_ptr<response::connection> conn, ws_config_t cfg)
			:conn_(std::move(conn)), buffer_(8192 + LONG_MESSAGE_HEADER), cfg_(std::move(cfg))
		{

		}

		boost::string_ref websocket_connection::is_websocket_handshake(const request& req)
		{
			// websocket的握手必须是GET方式
			if (req.method() != "GET")
			{
				return{};
			}

			// 包含upgrade字段,且值为不区分大小写的"websocket"
			auto upgrade_val = req.get_header("upgrade", 7);
			if (upgrade_val.empty() || !iequal(upgrade_val.data(), upgrade_val.size(), "websocket", 9))
			{
				return{};
			}

			/* sec-websocket-key header */
			auto sec_ws_key = req.get_header("sec-websocket-key", 17);
			if (sec_ws_key.size() != 24)
			{
				return{};
			}

			return sec_ws_key;
		}

		void websocket_connection::upgrade_to_websocket(
			request const& req, response& rep, boost::string_ref const& client_key, ws_config_t cfg)
		{
			static const char ws_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			uint8_t sha1buf[20], key_src[60];
			char accept_key[29];

			std::memcpy(key_src, client_key.data(), 24);
			std::memcpy(key_src + 24, ws_guid, 36);
			SHA1(key_src, sizeof(key_src), sha1buf);
			base64_encode(accept_key, sha1buf, sizeof(sha1buf), 0);

			rep.set_status(response::switching_protocols);
			//rep.add_header("reason", "Switching Protocols");
			rep.add_header("Upgrade", "WebSocket");
			rep.add_header("Connection", "Upgrade");
			rep.add_header("Sec-WebSocket-Accept", std::string(accept_key, 28));
			rep.add_header("content-length", "0");
			auto protocal_str = req.get_header("Sec-WebSocket-Protocol");
			if (!protocal_str.empty())
			{
				rep.add_header("Sec-WebSocket-Protocol", protocal_str.to_string());
			}

			auto conn = rep.get_connection();
			conn->async_write(nullptr, 0, [conn, cfg](boost::system::error_code const& ec, size_t)
			{
				if (ec)
				{
					if (cfg.on_error)
					{
						cfg.on_error(ec);
					}
					return;
				}


				auto ws_conn = boost::make_shared<websocket_connection>(conn, std::move(cfg));
				ws_conn->start();
			});

		}

		void websocket_connection::start()
		{
			auto self = this->shared_from_this();
			conn_->async_read_some(buffer_.data() + LONG_MESSAGE_HEADER, buffer_.size() - LONG_MESSAGE_HEADER, [self, this](boost::system::error_code const& ec, std::size_t length)
			{
				if (ec)
				{
					if (cfg_.on_error)
					{
						cfg_.on_error(ec);
					}
					return;
				}

				consume(buffer_.data() + LONG_MESSAGE_HEADER, length, nullptr);
				start();
			});
		}

		void websocket_connection::async_send_msg(const char* data, std::size_t length, opcode_t opCode, async_write_msg_callback_t handler)
		{
			auto buffers = format_message(data, length, opCode);
			conn_->async_write(buffers, [handler](boost::system::error_code const& ec, std::size_t)
			{
				handler(ec);
			});
		}

		void websocket_connection::close(int code, char *message, size_t length)
		{
			static const int MAX_CLOSE_PAYLOAD = 123;
			length = std::min<size_t>(MAX_CLOSE_PAYLOAD, length);
			if (cfg_.on_close)
			{
				cfg_.on_close(this->shared_from_this(), boost::string_ref(message, length), (opcode_t)code);
			}
			shutting_down = true;
//TODO: Timer
// 				startTimeout<WebSocket<isServer>::onEnd>();

// 			char close_payload[MAX_CLOSE_PAYLOAD + 2];
			boost::shared_array<char> close_payload(new char[MAX_CLOSE_PAYLOAD + 2]);
			std::size_t close_payload_length = format_close_payload(close_payload.get(), code, message, length);
			auto self = this->shared_from_this();
			async_send_msg(close_payload.get(), close_payload_length, opcode_t::CLOSE, [close_payload, self, this](boost::system::error_code const& ec)
			{
				if (ec)
				{
					conn_->close();
				}
				else
				{
					conn_->shutdown([](boost::system::error_code const&) {});
				}
			});
		}

		void websocket_connection::unmask_imprecise(char *dst, char *src, char *mask, std::size_t length)
		{
			for (std::size_t n = (length >> 2) + 1; n; n--)
			{
				*(dst++) = *(src++) ^ mask[0];
				*(dst++) = *(src++) ^ mask[1];
				*(dst++) = *(src++) ^ mask[2];
				*(dst++) = *(src++) ^ mask[3];
			}
		}

		void websocket_connection::unmask_imprecise_copy_mask(char *dst, char *src, char *mask_ptr, std::size_t length)
		{
			char mask[4] = { mask_ptr[0], mask_ptr[1], mask_ptr[2], mask_ptr[3] };
			unmask_imprecise(dst, src, mask, length);
		}

		void websocket_connection::unmask_inplace(char *data, char *stop, char *mask)
		{
			while (data < stop)
			{
				*(data++) ^= mask[0];
				*(data++) ^= mask[1];
				*(data++) ^= mask[2];
				*(data++) ^= mask[3];
			}
		}

		void websocket_connection::rotate_mask(unsigned int offset, char *mask)
		{
			char original_mask[4] = { mask[0], mask[1], mask[2], mask[3] };
			mask[(0 + offset) % 4] = original_mask[0];
			mask[(1 + offset) % 4] = original_mask[1];
			mask[(2 + offset) % 4] = original_mask[2];
			mask[(3 + offset) % 4] = original_mask[3];
		}

		cinatra::websocket::websocket_connection::close_frame_t websocket_connection::parse_close_payload(char *src, size_t length)
		{
			close_frame_t cf = {};
			if (length >= 2)
			{
				std::memcpy(&cf.code, src, 2);
				cf = { ntohs(cf.code), src + 2, length - 2 };
				if (cf.code < 1000 || cf.code > 4999 || (cf.code > 1011 && cf.code < 4000) ||
					(cf.code >= 1004 && cf.code <= 1006) || !is_valid_utf8((unsigned char *)cf.message, cf.length))
				{
					return{};
				}
			}
			return cf;
		}

		void websocket_connection::consume(char *src, std::size_t length, void *user)
		{
			if (spill_length)
			{
				src -= spill_length;
				length += spill_length;
				std::memcpy(src, spill, spill_length);
			}
			if (state == READ_HEAD)
			{
			parseNext:
				for (frame_format_t frame; length >= SHORT_MESSAGE_HEADER; )
				{
					std::memcpy(&frame, src, sizeof(frame_format_t));

					// invalid reserved bits / invalid opcodes / invalid control frames / set compressed frame
					if ((rsv1(frame) && !set_compressed(user)) || rsv23(frame) || (get_opcode(frame) > 2 && get_opcode(frame) < 8) ||
						get_opcode(frame) > 10 || (get_opcode(frame) > 2 && (!is_fin(frame) || payload_length(frame) > 125)))
					{
						force_close(user);
						return;
					}

					if (payload_length(frame) < 126)
					{
						if (consume_message<SHORT_MESSAGE_HEADER, uint8_t>(payload_length(frame), src, length, frame, user))
						{
							return;
						}
					}
					else if (payload_length(frame) == 126)
					{
						if (length < MEDIUM_MESSAGE_HEADER)
						{
							break;
						}
						else if (consume_message<MEDIUM_MESSAGE_HEADER, uint16_t>(ntohs(*(uint16_t *)&src[2]), src, length, frame, user))
						{
							return;
						}
					}
					else if (length < LONG_MESSAGE_HEADER)
					{
						break;
					}
					else if (consume_message<LONG_MESSAGE_HEADER, uint64_t>(be64toh(*(uint64_t *)&src[2]), src, length, frame, user))
					{
						return;
					}
				}
				if (length)
				{
					std::memcpy(spill, src, length);
					spill_length = length;
				}
			}
			else if (consume_continuation(src, length, user))
			{
				goto parseNext;
			}
		}

		bool websocket_connection::consume_continuation(char *&src, std::size_t &length, void *user)
		{
			if (remaining_bytes <= length)
			{
				std::size_t n = remaining_bytes >> 2;
				unmask_inplace(src, src + n * 4, mask);
				for (int i = 0, s = remaining_bytes % 4; i < s; i++)
				{
					src[n * 4 + i] ^= mask[i];
				}

				if (handle_fragment(src, remaining_bytes, 0, opcode[(unsigned char)op_stack], last_fin != 0, user))
				{
					return false;
				}

				if (last_fin)
				{
					op_stack--;
				}

				src += remaining_bytes;
				length -= remaining_bytes;
				state = READ_HEAD;
				return true;
			}
			else
			{
				unmask_inplace(src, src + ((length >> 2) + 1) * 4, mask);

				remaining_bytes -= length;
				if (handle_fragment(src, length, remaining_bytes, opcode[(unsigned char)op_stack], last_fin != 0, user))
				{
					return false;
				}

				if (length % 4)
				{
					rotate_mask(4 - (length % 4), mask);
				}
				return false;
			}
		}

		bool websocket_connection::handle_fragment(char *data, size_t length, std::size_t remaining_bytes, int opcode, bool fin, void *user)
		{
			if (opcode < 3)
			{
				if (!remaining_bytes && fin && !fragment_buffer.length())
				{
// 					if (webSocketData->compressionStatus == WebSocket<isServer>::Data::CompressionStatus::COMPRESSED_FRAME)
// 					{
// 						webSocketData->compressionStatus = WebSocket<isServer>::Data::CompressionStatus::ENABLED;
// 						Hub *hub = ((Group<isServer> *) s.getSocketData()->nodeData)->hub;
// 						data = hub->inflate(data, length);
// 						if (!data)
// 						{
// 							forceClose(user);
// 							return true;
// 						}
// 					}

					if (opcode == 1 && !is_valid_utf8((unsigned char *)data, length))
					{
						force_close(user);
						return true;
					}
					if (cfg_.on_message)
					{
						cfg_.on_message(this->shared_from_this(), boost::string_ref(data, length), (opcode_t)opcode);
					}
					if (is_closed() || is_shutting_down())
					{
						return true;
					}
				}
				else
				{
					fragment_buffer.append(data, length);
					if (!remaining_bytes && fin)
					{
						length = fragment_buffer.length();
// 						if (webSocketData->compressionStatus == WebSocket<isServer>::Data::CompressionStatus::COMPRESSED_FRAME)
// 						{
// 							webSocketData->compressionStatus = WebSocket<isServer>::Data::CompressionStatus::ENABLED;
// 							Hub *hub = ((Group<isServer> *) s.getSocketData()->nodeData)->hub;
// 							webSocketData->fragmentBuffer.append("....");
// 							data = hub->inflate((char *)webSocketData->fragmentBuffer.data(), length);
// 							if (!data)
// 							{
// 								forceClose(user);
// 								return true;
// 							}
// 						}
// 						else
// 						{
						data = &fragment_buffer[0];
// 						}

						if (opcode == 1 && !is_valid_utf8((unsigned char *)data, length))
						{
							force_close(user);
							return true;
						}
						if (cfg_.on_message)
						{
							cfg_.on_message(this->shared_from_this(), boost::string_ref(data, length), (opcode_t)opcode);
						}
						if (is_closed() || is_shutting_down())
						{
							return true;
						}
						fragment_buffer.clear();
					}
				}
			}
			else
			{
				// todo: we don't need to buffer up in most cases!
				control_buffer.append(data, length);
				if (!remaining_bytes && fin)
				{
					if (opcode == CLOSE)
					{
						close_frame_t close_frame = parse_close_payload(&control_buffer[0], control_buffer.length());
						close(close_frame.code, close_frame.message, close_frame.length);
						return true;
					}
					else
					{
						if (opcode == PING)
						{
							auto self = this->shared_from_this();
							async_send_msg(control_buffer, opcode_t::PONG, [self](boost::system::error_code const&) {});
							if (cfg_.on_ping)
							{
								cfg_.on_ping(this->shared_from_this(), boost::string_ref(control_buffer));
							}
							if (is_closed() || is_shutting_down())
							{
								return true;
							}
						}
						else if (opcode == PONG)
						{
							if (cfg_.on_pong)
							{
								cfg_.on_pong(this->shared_from_this(), boost::string_ref(control_buffer));
							}
							if (is_closed() || is_shutting_down())
							{
								return true;
							}
						}
					}
					control_buffer.clear();
				}
			}

			return false;
		}

		std::vector<boost::asio::const_buffer> websocket_connection::format_message(
			const char *src, size_t length, opcode_t opCode/*, size_t reportedLength, bool compressed*/)
		{
			size_t header_length;

			if (length < 126)
			{
				header_length = 2;
				msg_header_[1] = static_cast<char>(length);
			}
			else if (length <= UINT16_MAX)
			{
				header_length = 4;
				msg_header_[1] = 126;
				*((uint16_t *)&msg_header_[2]) = htons(static_cast<uint16_t>(length));
			}
			else
			{
				header_length = 10;
				msg_header_[1] = 127;
				*((uint64_t *)&msg_header_[2]) = htobe64(length);
			}

			int flags = 0;
			msg_header_[0] = (flags & SND_NO_FIN ? 0 : 128)/* | (compressed ? SND_COMPRESSED : 0)*/;
			if (!(flags & SND_CONTINUATION))
			{
				msg_header_[0] |= opCode;
			}

			return{ boost::asio::buffer(msg_header_, header_length), boost::asio::buffer(src, length) };
		}

		size_t websocket_connection::format_close_payload(char *dst, uint16_t code, char *message, size_t length)
		{
			if (code)
			{
				code = htons(code);
				std::memcpy(dst, &code, 2);
				std::memcpy(dst + 2, message, length);
				return length + 2;
			}
			return 0;
		}

	}

}

