/*
 * Common.h
 *
 *      Author: jc
 */

#pragma once

#include <string>
#include <algorithm>
#include <functional>


#ifdef _MSC_VER
#define snprintf _snprintf
#include <io.h>
#else
#include <unistd.h>
#endif


namespace cinatra
{
	// Taken from here http://stackoverflow.com/a/15167203/2080453
	inline std::string dbl2str(double d)
	{
		size_t len = snprintf(0, 0, "%.10f", d);
		std::string s(len + 1, 0);
		snprintf(&s[0], len + 1, "%.10f", d);
		s.pop_back();
		s.erase(s.find_last_not_of('0') + 1, std::string::npos);
		if (s.back() == '.')
		{
			s.pop_back();
		}

		return s;
	}

	inline std::string replaceString(std::string subject, const std::string & search, const std::string & replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	}

	inline void trimString(std::string & str)
	{
		str.erase(str.begin(), std::find_if(str.begin(), str.end(),
			std::not1(std::function< int(int) >(isspace))));
		str.erase(std::find_if(str.rbegin(), str.rend(),
			std::not1(std::function< int(int) >(isspace))).base(), str.end());
	}

	inline bool isReadableFile(std::string const & filePath)
	{
		//R_OK的值为4，VC没有定义R_OK
		return access(filePath.c_str(), 4) != -1;
	}

	typedef std::string(*StrConcat)(std::string const &, std::string const &);
	static StrConcat strConcat = std::operator+;

}

