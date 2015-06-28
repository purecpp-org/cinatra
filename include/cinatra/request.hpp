
#pragma once

#include <string>
#include <vector>
#include "utils.hpp"

namespace cinatra
{
	struct Request
	{
		std::string raw_url;
		std::vector<char> raw_body;
		std::string method;
		std::string path;
		CaseMap query;
		CaseMap body;
		NcaseMultiMap header;
	};
}