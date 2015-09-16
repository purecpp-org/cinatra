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
	class HTTPRouter
	{
		using invoker_function = std::function<bool(token_parser &)>;
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

			//parser_.add(funcName, v);
			name_param_map_.emplace(funcName, v);
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

		bool dispatch(Request& req,  Response& res, ContextContainer& ctx)
		{
			token_parser parser(req,res,ctx);
			
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
				bool r = handle(func_name, parser, finish);
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
						bool r = handle(name, parser, finish);
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
			auto rg = name_param_map_.equal_range(name);
			for (auto itr = rg.first; itr != rg.second; ++itr)
			{
				if (itr->second.size() == parser.size())
				{
					return true;
				}
			}

			return false;
		}

		bool handle(std::string& name, token_parser& parser, bool& finish)
		{
			bool r = false;
			auto it = map_invokers.equal_range(name);
			for (auto itr = it.first; itr != it.second; ++itr)
			{
				token_parser tmp = parser;
				tmp.param_error_ = false;
				r = itr->second(tmp);

				if (!tmp.param_error_)
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
				std::tuple<>()));
		}

		template<class Signature, typename Function, typename Self>
		void register_member_impl(const std::string& name, const Function& f, Self* self)
		{
			// instantiate and store the invoker by name
			this->map_invokers.emplace(name, std::bind(&invoker<Function, Signature>::template call_member<std::tuple<>, Self>, f, self, std::placeholders::_1, 
				std::tuple<>()));
		}

	private:
		template<typename ParamT,typename T=void>
		struct GetParam
		{
			using type = ParamT;
			static type get_param(token_parser& parser, bool& ret)
			{
				return parser.get<type>(ret);
			}
		};

		template<typename T>
		struct GetParam<Request,T>
		{
			using type = std::reference_wrapper<Request>;
			static type get_param(token_parser& parser, bool& ret)
			{
				ret = true;
				return std::ref(parser.get_req());
			}
		};
		template<typename T>
		struct GetParam<Response,T>
		{
			using type = std::reference_wrapper<Response>;
			static type get_param(token_parser& parser, bool& ret)
			{
				ret = true;
				return std::ref(parser.get_res());
			}
		};
		template<typename T>
		struct GetParam<ContextContainer,T>
		{
			using type = std::reference_wrapper<ContextContainer>;
			static type get_param(token_parser& parser, bool& ret)
			{
				ret = true;
				return std::ref(parser.get_ctx());
			}
		};

		template<typename Function, class Signature = Function, size_t N = function_traits<Signature>::arity>
		struct invoker;

		template<typename Function, class Signature, size_t N>
		struct invoker
		{
			// add an argument to a Fusion cons-list for each parameter type
			template<typename Args>
			static inline bool call(const Function& func, token_parser & parser, const Args& args)
			{
				if (N > parser.size() + 3 || N < parser.size())
				{
					parser.param_error_ = true;
					return false;
				}

				typedef typename function_traits<Signature>::template args<N-1>::type arg_type;
				bool ret;
				using G = GetParam<typename std::decay<arg_type>::type>;
				typename G::type param = G::get_param(parser, ret);
				if (!ret)
				{
					parser.param_error_ = true;
					return false;
				}
				return HTTPRouter::invoker<Function, Signature, N - 1>::call(func, parser, std::tuple_cat(std::make_tuple(param), args));
			}

			template<typename Args, typename Self>
			static inline bool call_member(Function func, Self* self, token_parser & parser, const Args& args)
			{
				if (N > parser.size() + 3 || N < parser.size())
				{
					parser.param_error_ = true;
					return false;
				}

				typedef typename function_traits<Signature>::template args<N-1>::type arg_type;
				bool ret;
				using G = GetParam<typename std::decay<arg_type>::type>;
				typename G::type param = G::get_param(parser, ret);
				if (!ret)
				{
					parser.param_error_ = true;
					return false;
				}
				return HTTPRouter::invoker<Function, Signature, N - 1>::call_member(func, self, parser, std::tuple_cat(std::make_tuple(param), args));
			}
		};

		template<typename Function, class Signature>
		struct invoker<Function, Signature, 0>
		{
			// the argument list is complete, now call the function
			template<typename Args>
			static inline bool call(const Function& func, token_parser &/*parser*/, const Args& args)
			{
				apply(func, args);
				return true;
			}

			template<typename Args, typename Self>
			static inline bool call_member(const Function& func, Self* self, token_parser &/*parser*/, const Args& args)
			{
				apply_member(func, self, args);
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
		static void apply_helper(const F& f, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)
		{
			f(std::get<Indexes>(tup)...);
		}

		template<typename F, typename ... Args>
		static void apply(const F& f, const std::tuple<Args...>& tp)
		{
			apply_helper(f, typename MakeIndexes<sizeof... (Args)>::type(), tp);
		}

		template<typename F, typename Self, int ... Indexes, typename ... Args>
		static void apply_helper_member(const F& f, Self* self, IndexTuple<Indexes...>, const std::tuple<Args...>& tup)
		{
			(*self.*f)(std::get<Indexes>(tup)...);
		}

		template<typename F, typename Self, typename ... Args>
		static void apply_member(const F& f, Self* self, const std::tuple<Args...>& tp)
		{
			apply_helper_member(f, self, typename MakeIndexes<sizeof... (Args)>::type(), tp);
		}

	private:
		std::multimap<std::string, invoker_function> map_invokers;
		std::multimap<std::string, std::vector<std::string>> name_param_map_;
	};
}
