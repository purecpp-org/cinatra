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
#include "request.hpp"
#include "response.hpp"

namespace cinatra
{
	class HttpRouter
	{
		typedef std::function<void(token_parser &)> invoker_function;

		std::map<std::string, invoker_function> map_invokers;

		const Request& req_;
		const Response& resp_;
		token_parser parser_;

		void add(int a, int b)
		{
			std::cout << a + b << std::endl;
			std::cout << req_.path() << std::endl;
		}

	public:
		HttpRouter(const Request& req, const Response& resp) : req_(req), resp_(resp)
		{
			//支持函数指针和成员函数
			assign("hello", [this](int a, int b){
				std::cout << req_.url() << std::endl;
			});

			assign("add", &HttpRouter::add);
		}

		HttpRouter() = default;

		template<typename Function>
		typename std::enable_if<!std::is_member_function_pointer<Function>::value>::type assign(const std::string& name, const Function& f)
		{
			//判断是否含有:，有则解析参数key然后给token_parser
			//先判断是否有匹配的，这里要注意非标准的情况
			//if (name.find_first_of(':') == std::string::npos)
			//{
			//	register_nonmenber_impl<Function>(name, f);
			//}
			//else
			//{

			//}
			register_nonmenber_impl<Function>(name, f); //对函数指针有效
		}

		template<typename Function>
		typename std::enable_if<std::is_member_function_pointer<Function>::value>::type assign(const std::string& name, const Function& f)
		{
			register_member_impl<Function, Function, HttpRouter>(name, f, this);
		}

		void remove_function(const std::string& name) {
			this->map_invokers.erase(name);
		}

		void dispatch(std::string& text)
		{
			//parser_.parse(req_);
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

		template<class Signature, typename Function, typename Self>
		void register_member_impl(const std::string& name, const Function& f, Self* self)
		{
			// instantiate and store the invoker by name
			this->map_invokers[name] = std::bind(&invoker<Function, Signature>::template call_member<std::tuple<>, Self>, f, self, std::placeholders::_1, std::tuple<>());
		}

	private:
		template<typename Function, class Signature = Function, size_t N = 0, size_t M = function_traits<Signature>::arity>
		struct invoker;

		template<typename Function, class Signature, size_t N, size_t M>
		struct invoker
		{
			// add an argument to a Fusion cons-list for each parameter type
			template<typename Args>
			static inline void call(const Function& func, token_parser & parser, const Args& args)
			{
				typedef typename function_traits<Signature>::template args<N>::type arg_type;
				HttpRouter::invoker<Function, Signature, N + 1, M>::call(func, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
			}

			template<typename Args, typename Self>
			static inline void call_member( Function func, Self* self, token_parser & parser, const Args& args)
			{
				typedef typename function_traits<Signature>::template args<N>::type arg_type;
				HttpRouter::invoker<Function, Signature, N + 1, M>::call_member(func, self, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
			}
		};

		template<typename Function, class Signature, size_t M>
		struct invoker<Function, Signature, M, M>
		{
			// the argument list is complete, now call the function
			template<typename Args>
			static inline void call(const Function& func, token_parser &, const Args& args)
			{
				apply(func, args);
			}

			template<typename Args, typename Self>
			static inline void call_member(const Function& func, Self* self, token_parser &, const Args& args)
			{
				apply_member(func, self, args);
			}
		};
	};
}

