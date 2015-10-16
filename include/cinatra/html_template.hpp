
#pragma once

#include <cinatra/http_server/response.hpp>

#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/template/file_template.hpp>

#include <string>

//html模板修改自RedZone，此头文件只是提供两个方便使用的函数
//具体使用方式和模板语法请参考官方文档
//https://github.com/jcfromsiberia/RedZone
//Thanks Ivan Il'in!
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