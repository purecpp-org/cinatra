#pragma once
#include "request.hpp"
#include "response.hpp"
namespace cinatra
{
	struct CheckLoginAspect
	{
		bool before(const Request& req, Response& res)
		{
			return true;
		}

		void after(const Request& req, Response& res)
		{

		}
	};
}
