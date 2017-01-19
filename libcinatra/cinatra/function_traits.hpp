#pragma once
#include <functional>
#include <tuple>
//普通函数.
//函数指针.
//function/lambda
//成员函数.
//函数对象.

//转换为std::function和函数指针.
template<typename T>
struct function_traits;

//普通函数.
template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
public:
	enum { arity = sizeof...(Args) };
	typedef Ret function_type(Args...);
	typedef Ret return_type;
	using stl_function_type = std::function<function_type>;
	typedef Ret(*pointer)(Args...);

	template<size_t I>
	struct args
	{
		static_assert(I < arity, "index is out of range, index must less than sizeof Args");
		using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
	};
};

//函数指针.
template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)>{};

//std::function
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)>{};

//member function
#define FUNCTION_TRAITS(...) \
	template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits<ReturnType(Args...)>{}; \

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)

//函数对象.
template<typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())>{};

template <typename Function>
typename function_traits<Function>::stl_function_type to_function(const Function& lambda)
{
	return static_cast<typename function_traits<Function>::stl_function_type>(lambda);
}

template <typename Function>
typename function_traits<Function>::stl_function_type to_function(Function&& lambda)
{
	return static_cast<typename function_traits<Function>::stl_function_type>(std::forward<Function>(lambda));
}

template <typename Function>
typename function_traits<Function>::pointer to_function_pointer(const Function& lambda)
{
	return static_cast<typename function_traits<Function>::pointer>(lambda);
}


// use tuple to invoke function
template<int...>
struct index_tuple {};

template<int N, int... Indexes>
struct make_indexes : make_indexes<N - 1, N - 1, Indexes...> {};

template<int... Indexes>
struct make_indexes<0, Indexes...>
{
	typedef index_tuple<Indexes...> type;
};

template<typename F, int ... Indexes, typename ... Args>
static void apply_helper(const F& f, index_tuple<Indexes...>, const std::tuple<Args...>& tup)
{
	f(std::get<Indexes>(tup)...);
}

template<typename F, typename ... Args>
static void apply(const F& f, const std::tuple<Args...>& tp)
{
	apply_helper(f, typename make_indexes<sizeof... (Args)>::type(), tp);
}

