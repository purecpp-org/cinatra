#pragma once
#include "request.hpp"
#include "response.hpp"
template<int...>
struct IndexTuple{};

template<int N, int... Indexes>
struct MakeIndexes : MakeIndexes<N - 1, N - 1, Indexes...>{};

template<int... indexes>
struct MakeIndexes<0, indexes...>
{
	typedef IndexTuple<indexes...> type;
};

template<typename F, int ... Indexes, typename ... Args>
static inline auto apply_helper(const F& f, Request& req, Response& res, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)->decltype(f(req, res, std::get<Indexes>(tup)...))
{
	return f(req, res, std::get<Indexes>(tup)...);
}

template<typename F, typename ... Args>
static inline auto apply(const F& f, Request& req, Response& res, const std::tuple<Args...>& tp)->decltype(apply_helper(f, req, res, typename MakeIndexes<sizeof... (Args)>::type(), tp))
{
	return apply_helper(f, req, res, typename MakeIndexes<sizeof... (Args)>::type(), tp);
}

template<typename F, int ... Indexes, typename ... Args>
static void call_helper1(F f, Request& req, Response& res, IndexTuple<Indexes...>, std::tuple<Args...> tup)
{
	f(req, res, std::get<Indexes>(tup)...);
}

template<typename F, typename ... Args>
static void call1(F f, Request& req, Response& res, std::tuple<Args...> tp)
{
	call_helper1(f, req, res, typename MakeIndexes<sizeof... (Args)>::type(), tp);
}

template<typename F, typename Self, int ... Indexes, typename ... Args>
static inline void apply_member_helper(const F& f, Self* self, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)//->decltype(self->f(std::get<Indexes>(tup)...))
{
	 (*self.*f)(std::get<Indexes>(tup)...);
}

template<typename F, typename Self, typename ... Args>
static inline void apply_member(const F& f, Self* self, const std::tuple<Args...>& tp)//->decltype(apply_member_helper(f, self, typename MakeIndexes<sizeof... (Args)>::type(), tp))
{
	 apply_member_helper(f, self, typename MakeIndexes<sizeof... (Args)>::type(), tp);
}
