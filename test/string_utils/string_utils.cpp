#include "UnitTest.hpp"
#include <cinatra/utils/string_utils.hpp>

#include <iostream>

TEST_CASE(string_utils_split_test)
{
	std::string s = "1:2:3";
	std::vector<std::string> res = StringUtil::split(s, ':');	
	TEST_CHECK(res[0] == "1");
	TEST_CHECK(res[1] == "2");
	TEST_CHECK(res[2] == "3");
}
