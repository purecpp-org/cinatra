/*
 * Exception.h
 *
 *  Created on: 2014
 *      Author: jc
 */
#pragma once

#include <stdexcept>
#include <string>

namespace cinatra
{
	class Exception : public std::exception
	{
	public:
		Exception(std::string const & message)
			: m_message(message){}

		virtual const char * what() const throw()
		{
			return m_message.c_str();
		}

	protected:
		std::string m_message;
	};


	class JsonError : public Exception
	{
	public:
		JsonError(std::string const & message)
			: Exception(message)
		{}
	};

	class TemplateContextError : public Exception
	{
	public:
		TemplateContextError(std::string const & contextVar)
			: Exception("Cannot resolve '" + contextVar + "'")
		{}
		virtual ~TemplateContextError()
		{}
	};


	class TemplateSyntaxError : public Exception
	{
	public:
		TemplateSyntaxError(std::string const & wrongSyntax)
			: Exception("'" + wrongSyntax + "' seems like invalid syntax.")
		{}
		virtual ~TemplateSyntaxError(){}
	};


	class ExpressionException : public Exception
	{
	public:
		using Exception::Exception;

		ExpressionException(std::string const & expr, std::string const & err)
			: Exception("In expression \"" + expr + "\": " + err)
		{}

		virtual ~ExpressionException(){}
	};


	class IOError : public Exception
	{
	public:
		IOError(std::string const & message)
			: Exception(message) {}
		virtual ~IOError(){}
	};

}

