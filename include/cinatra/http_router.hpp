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
	class HTTPRouter
	{
		typedef std::function<void(token_parser &)> invoker_function;
	public:
		HTTPRouter()
		{}

		template<typename Function>
		typename std::enable_if<!std::is_member_function_pointer<Function>::value>::type route(const std::string& name, const Function& f)
		{
			std::string funcName = getFuncName(name);

			register_nonmenber_impl<Function>(funcName, f); //对函数指针有效.
		}

		std::string getFuncName(std::string name)
		{
			size_t pos = name.find_first_of(':');
			if (pos == std::string::npos)
				return name;

			std::string funcName = name.substr(0, pos - 1);
			while (pos != string::npos)
			{
				//获取参数key，/hello/:name/:age
				size_t nextpos = name.find_first_of('/', pos);
				string paramKey = name.substr(pos + 1, nextpos - pos - 1);
				parser_.add(funcName, paramKey);
				pos = name.find_first_of(':', nextpos);
			}

			return funcName;
		}

		template<typename Function>
		typename std::enable_if<std::is_member_function_pointer<Function>::value>::type route(const std::string& name, const Function& f)
		{
			std::string funcName = getFuncName(name);
			register_member_impl<Function, Function, HTTPRouter>(funcName, f, this);
		}

		void remove_function(const std::string& name) {
			this->map_invokers.erase(name);
		}

		bool dispatch(const Request& req,  Response& resp)
		{
			req_ = &req;
			resp_ = &resp;
			parser_.parse(*req_);

			if (parser_.empty())
				return false;

			auto func = getFunction();
			if (func == nullptr)
				return false;

			func(parser_);
			return true;
		}

		//如果有参数key就按照key从query里取出相应的参数值.
		//如果没有则直接查找，需要逐步匹配，先匹配最长的，接着匹配次长的，直到查找完所有可能的path.
		invoker_function getFunction()
		{
			std::string func_name = parser_.get<std::string>();
			auto it = map_invokers.find(func_name);
			if (it != map_invokers.end())
				return it->second;

			//处理非标准的情况.
			size_t pos = func_name.rfind('/');
			while (pos != string::npos)
			{
				string name = func_name;
				if (pos!=0)
					name = func_name.substr(0, pos);
				auto it = map_invokers.find(name);
				if (it == map_invokers.end())
				{
					pos = func_name.rfind('/', pos - 1);
					if (pos == 0)
						return nullptr;
				}
				else
				{
					string params = func_name.substr(pos);
					parser_.parse(params);
					return it->second;
				}
			}

			return nullptr;
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
				HTTPRouter::invoker<Function, Signature, N + 1, M>::call(func, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
			}

			template<typename Args, typename Self>
			static inline void call_member(Function func, Self* self, token_parser & parser, const Args& args)
			{
				typedef typename function_traits<Signature>::template args<N>::type arg_type;
				HTTPRouter::invoker<Function, Signature, N + 1, M>::call_member(func, self, parser, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
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

	private:
		std::map<std::string, invoker_function> map_invokers;

		const Request* req_;
		Response* resp_;
		token_parser parser_;
	};
}

