
#pragma once

#include <string>
#include <functional>
#include "request.hpp"
#include "response.hpp"

namespace cinatra
{
	class Cinatra;

	class Router
	{
	public:
		Router()
			:method_(Request::method_t::UNKNOWN)
		{}
		Router& method(Request::method_t method)
		{
			method_ = method;
			return *this;
		}

		typedef std::function<void(const Request&, Response&)> handler_t;
		Router& set_handler(handler_t handler)
		{
			handler_ = handler;
			return *this;
		}

		Router& operator()(handler_t handler)
		{
			return set_handler(handler);
		}
	private:
		friend Cinatra;
		Router(const std::string& rule)
			:rule_(rule), method_(Request::method_t::UNKNOWN)
		{}

		bool handle(const Request& req, Response& res)
		{
			if (req.path() != rule_)
			{
				return false;
			}

			if (method_ != Request::method_t::UNKNOWN && method_ != req.method())
			{
				return false;
			}

			handler_(req, res);
			return true;
		}
	private:
		std::string rule_;
		Request::method_t method_;
		handler_t handler_;
	};
}