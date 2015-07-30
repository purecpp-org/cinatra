#include <boost/test/unit_test.hpp>
#include <cinatra/lexical_cast.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE(lexical_cast_bool_to_string)
{
	BOOST_CHECK(lexical_cast<std::string>(true) == "1");
	BOOST_CHECK(lexical_cast<std::string>(false) == "0");
}

BOOST_AUTO_TEST_CASE(lexical_cast_string_to_bool)
{
	BOOST_CHECK(lexical_cast<bool>("true") == true);
	BOOST_CHECK(lexical_cast<bool>("false") == false);
}

BOOST_AUTO_TEST_CASE(lexical_cast_interger_to_string)
{
	BOOST_CHECK(lexical_cast<std::string>(1) == "1");
	BOOST_CHECK(lexical_cast<std::string>(-1) == "-1");
	BOOST_CHECK(lexical_cast<std::string>(245) == "245");
}

BOOST_AUTO_TEST_CASE(lexical_cast_string_to_interger)
{
	BOOST_CHECK(lexical_cast<int>("1") == 1);
	BOOST_CHECK(lexical_cast<int>("-1") == -1);
	BOOST_CHECK(lexical_cast<long>("245") == 245);
}

BOOST_AUTO_TEST_CASE(lexical_cast_float_to_string)
{
	BOOST_CHECK(lexical_cast<std::string>(1.23).substr(0, 4) == "1.23");
	BOOST_CHECK(lexical_cast<std::string>(-1.23).substr(0, 5) == "-1.23");
	BOOST_CHECK(lexical_cast<std::string>(245.1).substr(0, 5) == "245.1");
}

BOOST_AUTO_TEST_CASE(lexical_cast_string_to_float)
{
	BOOST_CHECK(lexical_cast<float>("1.23") < 1.23 + 1E-5 && 
		lexical_cast<float>("1.23") > 1.23 - 1E-5);
	BOOST_CHECK(lexical_cast<double>("-1.23") < -1.23 + 1E-5 && 
		lexical_cast<double>("-1.23") > -1.23 - 1E-5);
	BOOST_CHECK(lexical_cast<long double>("245.1") < 245.1 + 1E-5 &&
		lexical_cast<double>("245.1") > 245.1 - 1E-5);
}
