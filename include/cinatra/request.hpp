
#pragma once

#include "http_parser_merged.h"
#include <string>
#include "utils.hpp"

namespace cinatra
{
	struct Request
	{
		std::string raw_url;
		std::string raw_body;
		http_method method;
		std::string path;
		CaseMap query;
		CaseMap body;
		NcaseMultiMap header;
	};
}