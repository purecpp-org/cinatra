#pragma once
#include <vector>
#include <string>

struct StringUtil
{
	inline static std::vector<std::string> split(std::string& s, char seperator)
	{
		std::vector<std::string> v;
		while (true)
		{
			auto pos = s.find(seperator, 0);
			if (pos == std::string::npos)
			{
				if (!s.empty())
					v.push_back(s);
				break;
			}
			if (pos != 0)
				v.push_back(s.substr(0, pos));
			s = s.substr(pos + 1, s.length());
		}

		return v;
	}

	inline static bool is_tspecial(int c)
	{
		switch (c)
		{
		case ' ': case '`': case '{': case '}': case '^': case '|':
			return true;
		default:
			return false;
		}
	}

	inline static bool is_char(int c)
	{
		return c >= 0 && c <= 127;
	}
};
