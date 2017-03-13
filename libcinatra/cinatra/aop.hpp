
#pragma once

#include <cinatra/context_container.hpp>

#include <cinatra_http/request.h>
#include <cinatra_http/response.h>

#include <tuple>

namespace cinatra
{
	template<int N>
	struct identity_t {};

	template <typename...Aspect>
	class aop
	{
	public:
		void set_func(std::function<bool(request const&, response&, context_container&)>&& func)
		{
			func_ = func;
		}


		bool invoke(request const& req, response& res, context_container& ctx)
		{
			invoke_before(req, res, ctx, identity_t<0>());
			if (!res.is_complete())
			{
				func_(req, res, ctx);
			}
			invoke_after(req, res, ctx, identity_t<0>());
			return res.is_complete() || res.is_delay();
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
		void invoke_before(request const& req, response& res, context_container& ctx, identity_t<N>)
		{
			std::get<N>(aspects_).before(req, res, ctx);
			invoke_before(req, res, ctx, identity_t<N+1>());
		}

		void invoke_before(request const&, response&, context_container&, identity_t<sizeof...(Aspect)>)
		{
		}


		template<int N>
		void invoke_after(request const& req, response& res, context_container& ctx, identity_t<N>)
		{
			std::get<N>(aspects_).after(req, res, ctx);
			invoke_after(req, res, ctx, identity_t<N+1>());
		}
		void invoke_after(request const&, response&, context_container&, identity_t<sizeof...(Aspect)>)
		{
		}

	private:
		std::tuple<Aspect...> aspects_;

		std::function<bool(request const&, response&, context_container&)> func_;
	};

}
