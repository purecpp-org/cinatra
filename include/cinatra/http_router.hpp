#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <cinatra/function_traits.hpp>
#include <cinatra/string_utils.hpp>
//#include <cinatra/tuple_utils.hpp>
#include <cinatra/token_parser.hpp>
#include <cinatra/request.hpp>
#include <cinatra/response.hpp>

namespace cinatra
{
	const static int PARAM_ERROR = -9999;
	class HTTPRouter
	{
		typedef std::function<bool(Request&, Response&, token_parser &)> invoker_function;
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
			{
				if (name!="/"&&name[name.length() - 1] == '/') //移除最后的'/'符
					return name.substr(0, name.length() - 1);

				return name;
			}

			std::string funcName = name.substr(0, pos - 1);
			std::vector<std::string> v;
			while (pos != std::string::npos)
			{
				//获取参数key，/hello/:name/:age
				size_t nextpos = name.find_first_of('/', pos);
				std::string paramKey = name.substr(pos + 1, nextpos - pos - 1);
				v.push_back(std::move(paramKey));
				
				pos = name.find_first_of(':', nextpos);
			}
			parser_.add(funcName, v);
			return funcName;
		}

		template<typename Function, typename Self>
		typename std::enable_if<std::is_member_function_pointer<Function>::value>::type route(const std::string& name, const Function& f, Self* self)
		{
			std::string funcName = getFuncName(name);
			register_member_impl<Function, Function, Self>(funcName, f, self);
		}

		void remove_function(const std::string& name) {
			this->map_invokers.erase(name);
		}

		bool dispatch(Request& req,  Response& resp)
		{
			token_parser parser;
			
			std::string func_name = req.path();
			if (func_name.empty())
			{
				return false;
			}

			bool finish = false;
			auto it = map_invokers.equal_range(func_name);
			if (it.first != it.second) //直接通过path找到对应的handler了，这个handler是没有参数的
			{
				//处理hello?name=a&age=12
				bool r = handle(req, resp, func_name, parser, finish);
				if (finish)
					return r;
			}
			else
			{
				//处理hello/a/12
				//先分离path，如果有参数key就按照key从query里取出相应的参数值.
				//如果没有则直接查找，需要逐步匹配，先匹配最长的，接着匹配次长的，直到查找完所有可能的path.
				bool needbreak = false;
				size_t pos = func_name.rfind('/');
				while (pos != std::string::npos &&!needbreak)
				{
					std::string name = func_name;
					if (pos != 0)
						name = func_name.substr(0, pos);
					else
					{
						name = "";
						needbreak = true;
					}

					std::string params = func_name.substr(pos);
					parser.parse(params);

					if (check(parser, name))
					{
						bool r = handle(req, resp, name, parser, finish);
						if (finish)
							return r;
					}

					pos = func_name.rfind('/', pos - 1);
				}
			}

			return false;
		}

		bool check(token_parser& parser, const std::string& name)
		{
			auto kv = parser_.get_map();
			auto rg = kv.equal_range(name);
			for (auto itr = rg.first; itr != rg.second; ++itr)
			{
				if (itr->second.size() == parser.size())
				{
					return true;
				}
			}

			return false;
		}

		bool handle(Request& req, Response& resp, std::string& name, token_parser& parser, bool& finish)
		{
			bool r = false;
			auto it = map_invokers.equal_range(name);
			for (auto itr = it.first; itr != it.second; ++itr)
			{
				auto it = resp.context().find(PARAM_ERROR);
				if (it != resp.context().end())
				{
					resp.context().erase(it);
				}
				r = itr->second(req, resp, parser);

				if (resp.context().find(PARAM_ERROR) == resp.context().end())
				{
					finish = true;
					break;
				}
			}

			return r;
		}

	public:
		template<class Signature, typename Function>
		void register_nonmenber_impl(const std::string& name, const Function& f)
		{
			// instantiate and store the invoker by name
			this->map_invokers.emplace(name, std::bind(&invoker<Function, Signature>::template call<std::tuple<>>, f, std::placeholders::_1,
				std::placeholders::_2, std::placeholders::_3,
				std::tuple<>()));
		}

		template<class Signature, typename Function, typename Self>
		void register_member_impl(const std::string& name, const Function& f, Self* self)
		{
			// instantiate and store the invoker by name
			this->map_invokers.emplace(name, std::bind(&invoker<Function, Signature>::template call_member<std::tuple<>, Self>, f, self, std::placeholders::_1, 
				std::placeholders::_2, std::placeholders::_3,
				std::tuple<>()));
		}

	private:
		template<typename Function, class Signature = Function, size_t N = function_traits<Signature>::arity>
		struct invoker;

		template<typename Function, class Signature, size_t N>
		struct invoker
		{
			// add an argument to a Fusion cons-list for each parameter type
			template<typename Args>
			static inline bool call(const Function& func, Request& req, Response& res, token_parser & parser, const Args& args)
			{
				if (N != parser.size() + 2)
				{
					res.context().emplace(PARAM_ERROR, 0);
					return false;
				}

				typedef typename function_traits<Signature>::template args<N-1>::type arg_type;
				typename std::decay<arg_type>::type param;
				if (!parser.get<arg_type>(param))
				{
					res.context().emplace(PARAM_ERROR, 0);
					return false;
				}
				return HTTPRouter::invoker<Function, Signature, N - 1>::call(func, req, res, parser, std::tuple_cat(std::make_tuple(param), args));
			}

			template<typename Args, typename Self>
			static inline bool call_member(Function func, Self* self, Request& req, Response& res, token_parser & parser, const Args& args)
			{
				if (N != parser.size() + 2)
				{
					res.context().emplace(PARAM_ERROR, 0);
					return false;
				}

				typedef typename function_traits<Signature>::template args<N-1>::type arg_type;
				typename std::decay<arg_type>::type param;
				if (!parser.get<arg_type>(param))
				{
					res.context().emplace(PARAM_ERROR, 0);
					return false;
				}
				return HTTPRouter::invoker<Function, Signature, N - 1>::call_member(func, self, req, res, parser, std::tuple_cat(std::make_tuple(param), args));
			}
		};

		template<typename Function, class Signature>
		struct invoker<Function, Signature, 2>
		{
			// the argument list is complete, now call the function
			template<typename Args>
			static inline bool call(const Function& func, Request& req, Response& res, token_parser &/*parser*/, const Args& args)
			{
				apply(func, req, res, args);
				return true;
			}

			template<typename Args, typename Self>
			static inline bool call_member(const Function& func, Self* self, Request& req, Response& res, token_parser &/*parser*/, const Args& args)
			{
				apply_member(func, self, req, res, args);
				return true;
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
		static void apply_helper(const F& f, Request& req, Response& res, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)
		{
			f(req, res, std::get<Indexes>(tup)...);
		}

		template<typename F, typename ... Args>
		static void apply(const F& f, Request& req, Response& res, const std::tuple<Args...>& tp)
		{
			apply_helper(f, req, res, typename MakeIndexes<sizeof... (Args)>::type(), tp);
		}

		template<typename F, typename Self, int ... Indexes, typename ... Args>
		static void apply_helper_member(const F& f, Self* self, Request& req, Response& res, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)
		{
			(*self.*f)(req, res, std::get<Indexes>(tup)...);
		}

		template<typename F, typename Self, typename ... Args>
		static void apply_member(const F& f, Self* self, Request& req, Response& res, const std::tuple<Args...>& tp)
		{
			apply_helper_member(f, self, req, res, typename MakeIndexes<sizeof... (Args)>::type(), tp);
		}

	private:
		std::multimap<std::string, invoker_function> map_invokers;
		token_parser parser_;
	};
}
