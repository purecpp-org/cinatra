#include <cinatra/router.h>


namespace cinatra
{
	bool router::handle(request const& req, response & res, context_container& ctx)
	{
		splited_path_t sp = split_path(req.path().to_string());
		for (auto handler : handlers_)
		{
			if (handler.params.size() != sp.size())
			{
				continue;
			}

			params_t params;
			std::size_t i = 0;
			for (; i < sp.size(); ++i)
			{
				auto str1 = sp[i];
				auto str2 = handler.params[i];
				if (!str2.empty() && str2.front() == ':')
				{
					params.push_back(str1);
					continue;
				}

				if (str1 != str2)
				{
					break;
				}
			}

			if (i == sp.size())
			{
				std::reverse(params.begin(), params.end());
				param_container con(std::move(params), req, res, ctx);
				return handler.handler(con);
			}
		}

		return false;
	}

}
