
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
			s.set_request_handler([this](const Request& req, Response& res)
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
	};
}