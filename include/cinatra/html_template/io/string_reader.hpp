/*
 * StringReader.h
 *
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/parser/parser.hpp>

#include <string>

namespace cinatra
{

	class StringReader : public Reader
	{
	public:
		StringReader(std::string const & string)
			: m_string(string), m_iter(m_string.begin()){}

		virtual std::string read(size_t nBytes)
		{
			std::string result(m_iter, m_iter + nBytes);
			m_iter += nBytes;
			return result;
		}
		virtual std::string readAll()
		{
			return m_string;
		}
		virtual std::string id() const{ return "<string>"; }

		virtual ~StringReader(){}

	protected:
		std::string const & m_string;
		std::string::const_iterator m_iter;
	};

}
