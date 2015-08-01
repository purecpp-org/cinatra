#pragma once
#include <cinatra/string_utils.hpp>
#include <cinatra/request.hpp>
#include <cinatra/utils.hpp>

#include <boost/lexical_cast.hpp>

#include <map>
class token_parser
{
	std::vector<std::string> v_; //解析之后，v_的第一个元素为函数名，后面的元素均为参数.
	std::multimap<std::string, std::vector<std::string>> map_;
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

	token_parser() = default;

	void add(const std::string& path, std::vector<std::string>& v)
	{
		map_.emplace(path, std::move(v));
	}

	const std::multimap<std::string, std::vector<std::string>>& get_map()
	{
		return map_;
	}

	void parse(std::string& params)
	{
		v_ = StringUtil::split(params, '/');
	}

public:
	std::string get_function_name()
	{
		if (v_.empty())
			return "";

		auto it = v_.begin();
		std::string name = *it;
		v_.erase(it);
		return name;
	}

	template<typename RequestedType>
	bool get(typename std::decay<RequestedType>::type& param)
	{
		if (v_.empty())
		{
			LOG_DBG << "Params is invalid ";
			return false;
		}

		try
		{
			typedef typename std::decay<RequestedType>::type result_type;
			auto const & v = v_.back();
			param = boost::lexical_cast<typename std::decay<result_type>::type>(v);
			v_.pop_back();
			return true;
		}
		catch (std::exception& e)
		{
			LOG_DBG << "Error in param converting: " << e.what();
		}
		return false;
	}

	bool empty(){ return v_.empty(); }

	size_t size() const
	{
		return v_.size();
	}
};
