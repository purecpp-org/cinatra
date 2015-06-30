
#pragma once

#include "http_server.hpp"
#include "router.hpp"

#include <string>
#include <vector>


namespace cinatra
{
	class Cinatra
	{
	public:
		template<typename Rule>
		Router& route(const Rule& rule)
		{
			routers_.push_back(Router(rule));
			return *(routers_.end() - 1);
		}

		Cinatra& threads(int num)
		{
			num_threads_ = num < 1 ? 1 : num;
			return *this;
		}

		Cinatra& error_handler(error_handler_t error_handler)
		{
			error_handler_ = error_handler;
			return *this;
		}

		Cinatra& listen(const std::string& address, const std::string& port)
		{
			listen_addr_ = address;
			listen_port_ = port;
			return *this;
		}

		Cinatra& listen(const std::string& address, unsigned short port)
		{
			listen_addr_ = address;
			listen_port_ = boost::lexical_cast<std::string>(port);
			return *this;
		}

		Cinatra& public_dir(const std::string& dir)
		{
			public_dir_ = dir;
			return *this;
		}

		void run()
		{
			HTTPServer s(num_threads_);
			s.set_request_handler([this](Request& req, Response& res)
			{
				for (auto router : routers_)
				{
					if (router.handle(req, res))
					{
						return true;
					}
				}

				return false;
			})
				.set_error_handler([this](int code, const std::string& msg, Request& req, Response& res)
			{
				if (error_handler_
					&& error_handler_(code,msg,req,res))
				{
					return true;
				}


				std::string html;
				auto s = status_header(code);
				html = "<html><head><title>" + s.second + "</title></head>";
				html += "<body>";
				html += "<h1>" + boost::lexical_cast<std::string>(s.first) + " " + s.second + " " + "</h1>";
				if (!msg.empty())
				{
					html += "<br> <h2>Message: " + msg + "</h2>";
				}
				html += "</body></html>";

				res.set_status_code(s.first);

				res.write(html);

				return true;
			})
				.public_dir(public_dir_)
				.listen(listen_addr_, listen_port_)
				.run();
		}
	private:
		std::vector<Router> routers_;
		int num_threads_;
		std::string listen_addr_;
		std::string listen_port_;
		std::string public_dir_;

		error_handler_t error_handler_;
	};
}