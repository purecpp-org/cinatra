/*
 * Reader.h
 *
 *      Author: jc
 */

#pragma once

#include <string>

namespace cinatra
{
	class Reader
	{
	public:
		virtual std::string read(size_t nBytes) = 0;
		virtual std::string readAll() = 0;
		virtual std::string id() const = 0;

		virtual ~Reader(){}

	protected:
		Reader(){}
	};

}
