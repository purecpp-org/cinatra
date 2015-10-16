/*
 * StringWriter.h
 *
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/io/writer.hpp>

#include <string>


namespace cinatra
{

	class StringWriter : public Writer
	{
	public:
		StringWriter(std::string & string)
			: m_string(string){}

		virtual void write(std::string const & data)
		{
			std::copy(data.begin(), data.end(), std::back_inserter(m_string));
		}
		virtual void flush(){}

		virtual ~StringWriter(){}

	protected:
		std::string & m_string;
	};

}

