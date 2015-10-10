#pragma once
#include <cinatra/utils/string_utils.hpp>
#include <cinatra/utils/utils.hpp>
#include <cinatra/http_server/request.hpp>
#include <cinatra/http_server/response.hpp>
#include <cinatra/context_container.hpp>
#include <cinatra/utils/logging.hpp>

#include <boost/lexical_cast.hpp>

#include <map>

namespace cinatra
{
	class token_parser
	{
	public:
		/*
		get("/hello/:name", (request, response) -> {
		return "Hello: " + request.params(":name");
		});
		hello/a/b->hello/a/b();hello/a(b);hello(a, b);
		*/
		//token_parser(std::string& s, char seperator)
		//{
		//	v_ = StringUtil::split(s, seperator);
		//}

		token_parser(Request& req, Response& res, ContextContainer& ctx)
			:req_(req), res_(res), ctx_(ctx)
		{}

		// 	void add(const std::string& path, std::vector<std::string>& v)
		// 	{
		// 		map_.emplace(path, std::move(v));
		// 	}

		// 	const std::multimap<std::string, std::vector<std::string>>& get_map()
		// 	{
		// 		return map_;
		// 	}

		void parse(std::string& params)
		{
			v_ = StringUtil::split(params, '/');
		}

		std::string get_function_name()
		{
			if (v_.empty())
				return "";

			auto it = v_.begin();
			std::string name = *it;
			v_.erase(it);
			return name;
		}

// 		template<typename RequestedType>
// 		bool get(RequestedType& param)
// 		{
// 			if (v_.empty())
// 			{
// 				LOG_DBG << "Params is invalid ";
// 				return false;
// 			}
// 
// 			try
// 			{
// 				auto const & v = v_.back();
// 				param = boost::lexical_cast<typename std::decay<RequestedType>::type>(v);
// 				v_.pop_back();
// 				return true;
// 			}
// 			catch (std::exception& e)
// 			{
// 				LOG_DBG << "Error in param converting: " << e.what();
// 			}
// 			return false;
// 		}

		template<typename RequestedType>
		RequestedType get(bool& ret)
		{
			if (v_.empty())
			{
				LOG_DBG << "Params is invalid ";
				ret = false;
				return RequestedType();
			}

			try
			{
				auto const & v = v_.back();
				RequestedType tmp = boost::lexical_cast<typename std::decay<RequestedType>::type>(v);
				v_.pop_back();
				ret = true;
				return tmp;
			}
			catch (std::exception& e)
			{
				LOG_DBG << "Error in param converting: " << e.what();
			}

			ret = false;
			return RequestedType();
		}

		Request& get_req(){ return req_; }
		Response& get_res(){ return res_; }
		ContextContainer& get_ctx(){ return ctx_; }

		bool empty(){ return v_.empty(); }

		size_t size() const
		{
			return v_.size();
		}

		bool param_error_ = false;
	private:
		Request& req_;
		Response& res_;
		ContextContainer& ctx_;
		std::vector<std::string> v_; //解析之后，v_的第一个元素为函数名，后面的元素均为参数.
		//std::multimap<std::string, std::vector<std::string>> map_;
	};
}
