#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include "function_traits.hpp"
#include "lexical_cast.hpp"

class HttpRouter
{
	class token_parser
	{
		std::vector<std::string> m_v;
		static std::vector<std::string> split(std::string& s, char seperator)
		{
			std::vector<std::string> v;
			int pos = 0;
			while (true)
			{
				pos = s.find(seperator, 0);
				if (pos == std::string::npos)
				{
					if (!s.empty())
						v.push_back(s);
					break;
				}
				if (pos != 0)
					v.push_back(s.substr(0, pos));
				s = s.substr(pos + 1, s.length());
			}

			return v;
		}
	public:

		token_parser(std::string& s, char seperator)
		{
			m_v = split(s, seperator);
		}

	public:
		template<typename RequestedType>
		typename std::decay<RequestedType>::type get()
		{
			if (m_v.empty())
				throw std::runtime_error("unexpected end of input");

			try
			{
				typedef typename std::decay<RequestedType>::type result_type;

				auto it = m_v.begin();
				result_type result = lexical_cast<typename std::decay<result_type>::type>(*it);
				m_v.erase(it);
				return result;
			}
			catch (std::exception& e)
			{
				throw std::invalid_argument(std::string("invalid argument: ") + e.what());
			}
		}

		bool empty(){ return m_v.empty(); }
	};

	typedef std::function<void(token_parser &)> invoker_function;

	std::map<std::string, invoker_function> map_invokers;

public:
	std::function<void(const std::string&)> log;

	template<typename Function>
	void assign(std::string const & name, Function f) {
		return register_nonmenber_impl<Function>(name, f);
	}

	void remove_function(std::string const& name) {
		this->map_invokers.erase(name);
	}

	void run(std::string  & text) const
	{
		token_parser parser(text, '/');

		while (!parser.empty())
		{
			// read function name
			std::string func_name = parser.get<std::string>();

			// look up function
			auto it = map_invokers.find(func_name);
			if (it == map_invokers.end())
				throw std::runtime_error("unknown function: " + func_name);

			// call the invoker which controls argument parsing
			it->second(parser);
		}
	}


public:
	template<class Signature, typename Function>
	void register_nonmenber_impl(std::string const & name, Function f)
	{
		// instantiate and store the invoker by name
		this->map_invokers[name] = std::bind(
			&invoker<Function, Signature>::template apply<std::tuple<>>,
			f,
			std::placeholders::_1,
			std::tuple<>()
			);
	}

private:
	template<typename Function, class Signature = Function, size_t N = 0, size_t M = function_traits<Signature>::arity>
	struct invoker;

	template<typename Function, class Signature, size_t N, size_t M>
	struct invoker
	{
		// add an argument to a Fusion cons-list for each parameter type
		template<typename Args>
		static inline void apply(Function func, token_parser & parser, Args const & args)
		{
			typedef typename function_traits<Signature>::template args<N>::type arg_type;
			HttpRouter::invoker<Function, Signature, N + 1, M>::apply
				(func, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
		}
	};

	template<typename Function, class Signature, size_t M>
	struct invoker<Function, Signature, M, M>
	{
		// the argument list is complete, now call the function
		template<typename Args>
		static inline void apply(Function func, token_parser &, Args const & args)
		{
			call(func, args);
		}
	};

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
	static void call_helper(F f, IndexTuple<Indexes...>, std::tuple<Args...> tup)
	{
		f(std::get<Indexes>(tup)...);
	}

	template<typename F, typename ... Args>
	static void call(F f, std::tuple<Args...> tp)
	{
		call_helper(f, typename MakeIndexes<sizeof... (Args)>::type(), tp);
	}

};
