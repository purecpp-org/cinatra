#pragma once
#include <vector>
#include <string>

inline std::vector<std::string> split(std::string& s, char seperator)
{
	std::vector<std::string> v;
	int pos = 0;
	while (true)
	{
		pos = s.find(seperator, 0);
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