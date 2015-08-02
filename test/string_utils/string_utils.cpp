#include <boost/test/unit_test.hpp>
#include <cinatra/string_utils.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE(string_utils_split_test)
{
	std::string s = "1:2:3";
	std::vector<std::string> res = StringUtil::split(s, ':');	
	BOOST_CHECK(res[0] == "1");
	BOOST_CHECK(res[1] == "2");
	BOOST_CHECK(res[2] == "3");
}
