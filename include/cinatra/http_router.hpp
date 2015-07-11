#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include "function_traits.hpp"
#include "lexical_cast.hpp"
#include "string_utils.hpp"
#include "tuple_utils.hpp"
#include "token_parser.hpp"

class HttpRouter
{
	typedef std::function<std::string(token_parser &)> invoker_function;

	std::map<std::string, invoker_function> map_invokers;

public:
	std::function<std::string(const std::string&)> log;

	template<typename Function>
	void assign(const std::string& name, const Function& f) {
		register_nonmenber_impl<Function>(name, f);
	}

	void remove_function(const std::string& name) {
		this->map_invokers.erase(name);
	}

	void dispatch(std::string& text) const
	{
		token_parser parser(text, '/');

		while (!parser.empty())
		{
			// read function name
			std::string func_name = parser.get<std::string>();

			// look up function
			auto it = map_invokers.find(func_name);
			if (it == map_invokers.end())
				throw std::invalid_argument("unknown function: " + func_name);

			// call the invoker which controls argument parsing
			it->second(parser);
		}
	}


public:
	template<class Signature, typename Function>
	void register_nonmenber_impl(const std::string& name, const Function& f)
	{
		// instantiate and store the invoker by name
		this->map_invokers[name] = std::bind(&invoker<Function, Signature>::template call<std::tuple<>>, f, std::placeholders::_1, std::tuple<>());
	}

private:
	template<typename Function, class Signature = Function, size_t N = 0, size_t M = function_traits<Signature>::arity>
	struct invoker;

	template<typename Function, class Signature, size_t N, size_t M>
	struct invoker
	{
		// add an argument to a Fusion cons-list for each parameter type
		template<typename Args>
		static inline string call(const Function& func, token_parser & parser, const Args& args)
		{
			typedef typename function_traits<Signature>::template args<N>::type arg_type;
			return HttpRouter::invoker<Function, Signature, N + 1, M>::call(func, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
		}
	};

	template<typename Function, class Signature, size_t M>
	struct invoker<Function, Signature, M, M>
	{
		// the argument list is complete, now call the function
		template<typename Args>
		static inline string call(const Function& func, token_parser &, const Args& args)
		{
			return apply(func, args);
		}
	};
};
