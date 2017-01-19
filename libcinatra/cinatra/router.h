#pragma once

#include <cinatra_http/request.h>
#include <cinatra_http/response.h>

#include <cinatra/function_traits.hpp>
#include <cinatra/context_container.hpp>

#include <boost/lexical_cast/try_lexical_convert.hpp>

#include <string>


namespace cinatra
{
	class router
	{
	private:
		using params_t = std::vector<std::string>;
		using splited_path_t = std::vector<std::string>;

		class param_container
		{
		public:
			explicit param_container(params_t params, request const& req, response& res, context_container& ctx)
				:params_(std::move(params)), req_(req), res_(res), ctx_(ctx)
			{
			}

			template<typename T>
			T get()
			{
				T val;
				if (cur_index_ >= params_.size())
				{
					success_ = false;
					return val;
				}

				success_ = boost::conversion::try_lexical_convert(params_[cur_index_], val);
				cur_index_++;
				return val;
			}

			request const& get_req() { return req_; }
			response& get_res() { return res_; }
			context_container& get_ctx() { return ctx_; }


			
			std::size_t remain_size()
			{
				return params_.size() - cur_index_;
			}

			void set_error()
			{
				success_ = false;
			}

			bool success()
			{
				return success_;
			}

			void reset()
			{
				cur_index_ = 0;
				success_ = true;
			}

		private:
			params_t params_;
			std::size_t cur_index_ = 0;
			bool success_ = true;
			request const& req_;
			response& res_;
			context_container& ctx_;
		};


		using handler_t = std::function<bool(param_container&)>;
		struct _splited_path_handler_t
		{
			splited_path_t params;
			handler_t handler;
		};


		template<typename Function, class Signature = Function, size_t N = function_traits<Signature>::arity>
		struct invoker;

		template<typename Function, class Signature, size_t N>
		struct invoker
		{
			// add an argument to a Fusion cons-list for each parameter type
			template<typename Args>
			static inline bool call(const Function& func, param_container& parser, const Args& args)
			{
				if (N > parser.remain_size() + 3 || N < parser.remain_size())
				{
					parser.set_error();
					return false;
				}

				typedef typename function_traits<Signature>::template args<N - 1>::type arg_type;
				using _G = _get_param_t<typename std::decay<arg_type>::type>;
				auto param = _G::get_param(parser);
				if (!parser.success())
				{
					return false;
				}
				return router::invoker<Function, Signature, N - 1>::call(func, parser, std::tuple_cat(std::make_tuple(param), args));
			}
		};

		template<typename Function, class Signature>
		struct invoker<Function, Signature, 0>
		{
			// the argument list is complete, now call the function
			template<typename Args>
			static inline bool call(const Function& func, param_container&/*parser*/, const Args& args)
			{
				apply(func, args);
				return true;
			}
		};

		template<typename ParamT, typename T = void>
		struct _get_param_t
		{
			using type = ParamT;
			static type get_param(param_container& params)
			{
				return params.get<type>();
			}
		};

		template<typename T>
		struct _get_param_t<request, T>
		{
			using type = std::reference_wrapper<request>;
			static type get_param(param_container& params)
			{
				return std::ref(params.get_req());
			}
		};
		template<typename T>
		struct _get_param_t<response, T>
		{
			using type = std::reference_wrapper<response>;
			static type get_param(param_container& params)
			{
				return std::ref(params.get_res());
			}
		};
		template<typename T>
		struct _get_param_t<context_container, T>
		{
			using type = std::reference_wrapper<context_container>;
			static type get_param(param_container& params)
			{
				return std::ref(params.get_ctx());
			}
		};


		splited_path_t split_path(std::string const& path)
		{
			std::size_t pos = 1;
			splited_path_t sp;

			for (;;)
			{
				auto next_pos = path.find('/', pos);
				if (next_pos == std::string::npos)
				{
					next_pos = path.size();
					if (next_pos != pos)
					{
						sp.emplace_back(path.substr(pos, next_pos - pos));
					}
					return sp;
				}
				sp.emplace_back(path.substr(pos, next_pos - pos));
				pos = next_pos + 1;
			}
		}

		std::vector<_splited_path_handler_t> handlers_;

	public:
		bool handle(request const& req, response & res, context_container& ctx);
		template<typename FunctionT>
		void route(const std::string& path, const FunctionT& f)
		{
			splited_path_t sp = split_path(path);
			handlers_.emplace_back(_splited_path_handler_t
			{
				std::move(sp),
				std::bind(&invoker<FunctionT>::template call<std::tuple<>>, f, std::placeholders::_1, std::tuple<>())
			});
		}
	};

}