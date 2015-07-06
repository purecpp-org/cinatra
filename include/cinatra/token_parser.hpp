#pragma once
#include "string_utils.hpp"
class token_parser
{
	std::vector<std::string> m_v;
public:

	token_parser(std::string& s, char seperator)
	{
		m_v = StringUtil::split(s, seperator);
	}

public:
	template<typename RequestedType>
	typename std::decay<RequestedType>::type get()
	{
		if (m_v.empty())
			throw std::invalid_argument("unexpected end of input");

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