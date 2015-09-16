
#pragma once

#include <cinatra/request.hpp>
#include <cinatra/response.hpp>
#include <cinatra/context_container.hpp>

#include <tuple>

namespace cinatra
{
	template <typename...Aspect>
	class AOP
	{
	public:
		void set_func(std::function<bool(Request&, Response&, ContextContainer&)>&& func)
		{
			func_ = func;
		}


		bool invoke(Request& req, Response& res, ContextContainer& ctx)
		{
			invoke_before<0>(req, res, ctx);
			bool ret = false;
			if (res.is_complete())
			{
				ret = true;
			}
			else
			{
				ret = func_(req, res, ctx);
			}
			invoke_after<0>(req, res, ctx);
			return ret;
		}

	private:
		template<int N>
		void invoke_before(Request& req, Response& res, ContextContainer& ctx)
		{
			std::get<N>(aspects_).before(req, res, ctx);
			invoke_before<N + 1>(req, res, ctx);
		}
		template<>
		void invoke_before<sizeof...(Aspect)>(Request&, Response&, ContextContainer&)
		{
		}


		template<int N>
		void invoke_after(Request& req, Response& res, ContextContainer& ctx)
		{
			std::get<N>(aspects_).after(req, res, ctx);
			invoke_after<N + 1>(req, res, ctx);
		}
		template<>
		void invoke_after<sizeof...(Aspect)>(Request&, Response&, ContextContainer&)
		{
		}

	private:
		std::tuple<Aspect...> aspects_;

		std::function<bool(Request&, Response&, ContextContainer&)> func_;
	};

}