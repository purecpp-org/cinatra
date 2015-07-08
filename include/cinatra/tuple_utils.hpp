#pragma once
#include <tuple>

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
static inline void apply_helper(const F& f, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)
{
	f(std::get<Indexes>(tup)...);
}

template<typename F, typename ... Args>
static inline void apply(const F& f, const std::tuple<Args...>& tp)
{
	apply_helper(f, typename MakeIndexes<sizeof... (Args)>::type(), tp);
}
