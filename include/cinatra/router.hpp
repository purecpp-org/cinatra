
#pragma once

#include <unordered_map>
#include <functional>

namespace cinatra
{
	class Request;
	class Response;

	class Router
	{
	public:
		Router();
		~Router();

	private:
		typedef std::function<void(Request&, Response&)> handler_t;
		std::unordered_map<std::string, handler_t> disp_map_;
	};
}