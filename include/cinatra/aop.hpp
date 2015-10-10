
#pragma once

#include <cinatra/request.hpp>
#include <cinatra/response.hpp>
#include <cinatra/context_container.hpp>

#include <tuple>

namespace cinatra
{
	template<int N>
	struct Identity {};

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
			invoke_before(req, res, ctx, Identity<0>());
			bool ret = false;
			if (res.is_complete())
			{
				ret = true;
			}
			else
			{
				ret = func_(req, res, ctx);
			}
			invoke_after(req, res, ctx, Identity<0>());
			return ret;
		}

		// 根据类型获取tuple中的实例
		template<int Index, class Search, class First, class... Types>
		struct get_internal
		{
			using type = typename get_internal<Index + 1, Search, Types...>::type;
			enum { index = Index };
		};

		template<int Index, class Search, class... Types>
		struct get_internal<Index, Search, Search, Types...>
		{
			using type = get_internal;
			enum { index = Index };
		};

		template<class T, class... Types>
		T& get_by_type(std::tuple<Types...>& tuple)
		{
			return std::get<get_internal<0, T, Types...>::type::index>(tuple);
		}

		template<typename T>
		T& get_aspect()
		{
			return get_by_type<T>(aspects_);
		}
	private:
		template<int N>
		void invoke_before(Request& req, Response& res, ContextContainer& ctx, Identity<N>)
		{
			std::get<N>(aspects_).before(req, res, ctx);
			invoke_before(req, res, ctx, Identity<N+1>());
		}
		void invoke_before(Request&, Response&, ContextContainer&, Identity<sizeof...(Aspect)>)
		{
		}


		template<int N>
		void invoke_after(Request& req, Response& res, ContextContainer& ctx, Identity<N>)
		{
			std::get<N>(aspects_).after(req, res, ctx);
			invoke_after(req, res, ctx, Identity<N+1>());
		}
		void invoke_after(Request&, Response&, ContextContainer&, Identity<sizeof...(Aspect)>)
		{
		}

	private:
		std::tuple<Aspect...> aspects_;

		std::function<bool(Request&, Response&, ContextContainer&)> func_;
	};

}