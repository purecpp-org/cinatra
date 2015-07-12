#pragma once
#include <map>
#include "string_utils.hpp"
#include "request.hpp"
class token_parser
{
	std::vector<std::string> v_; //解析之后，v_的第一个元素为函数名，后面的元素均为参数
	std::multimap<std::string, std::string> map_;
public:
	/*
	get("/hello/:name", (request, response) -> {
		return "Hello: " + request.params(":name");
	});
	hello/a/b->hello/a/b();hello/a(b);hello(a, b);
	*/
	token_parser(std::string& s, char seperator)
	{
		v_ = StringUtil::split(s, seperator);
	}

	token_parser() = default;

	void add(const std::string& path, const std::string& key)
	{
		map_.emplace(path, key);
	}

	void parse(const cinatra::Request& req)
	{
		//如果有参数key就按照key从query里取出相应的参数值
		//如果没有则直接查找，需要逐步匹配，先匹配最长的，接着匹配次长的，直到查找完所有可能的path
		string path = req.path();
		
		if (!map_.empty())
		{
			v_.push_back(req.path());
			for (auto& it : map_)
			{
				auto& val = req.query().get_val(it.second);
				if (val.empty())
					break;

				v_.push_back(val);
			}
		}
		else
		{
			v_ = StringUtil::split(path, '/');
		}
	}

public:
	template<typename RequestedType>
	typename std::decay<RequestedType>::type get()
	{
		if (v_.empty())
			throw std::invalid_argument("params is invalid ");

		try
		{
			typedef typename std::decay<RequestedType>::type result_type;

			auto it = v_.begin();
			result_type result = lexical_cast<typename std::decay<result_type>::type>(*it);
			v_.erase(it);
			return result;
		}
		catch (std::exception& e)
		{
			throw std::invalid_argument(std::string("invalid path: ") + e.what());
		}
	}

	bool empty(){ return v_.empty(); }
};