#pragma once
#include "string_utils.hpp"
class token_parser
{
	std::vector<std::string> v_; //解析之后，v_的第一个元素为函数名，后面的元素均为参数
public:

	token_parser(std::string& s, char seperator)
	{
		v_ = StringUtil::split(s, seperator);
	}

public:
	template<typename RequestedType>
	typename std::decay<RequestedType>::type get()
	{
		if (v_.empty())
			throw std::invalid_argument("unexpected end of input");

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
			throw std::invalid_argument(std::string("invalid argument: ") + e.what());
		}
	}

	bool empty(){ return v_.empty(); }
};