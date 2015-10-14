
#pragma once

#include <cinatra/http_server/response.hpp>

#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/template/file_template.hpp>

#include <string>

namespace cinatra
{
	inline void render(Response& res, const std::string& tpl_path, const Json& json)
	{
		Context ctx(json);
		FileTemplate tpl(tpl_path);

		res.end(tpl.render(&ctx));
	}
	inline void render(Response& res, const std::string& tpl_path, Context& ctx)
	{
		FileTemplate tpl(tpl_path);

		res.end(tpl.render(&ctx));
	}
}