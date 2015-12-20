#include "UnitTest.hpp"
#include <cinatra/utils/utils.hpp>

#include <iostream>

using namespace cinatra;

TEST_CASE(ncase_multi_map_test)
{
	NcaseMultiMap nmm;
	nmm.add("k1", "v1");
	nmm.add("K1", "V1");
	TEST_CHECK(nmm.get_count("k1") == 2);
	TEST_CHECK(nmm.get_count("K1") == 2);
	TEST_CHECK(nmm.get_val("k1") == "v1");
	TEST_CHECK(nmm.get_val("K1") == "v1");
	TEST_CHECK(nmm.val_ncase_equal("K1","V1"));
	TEST_CHECK(nmm.get_vals("k1").size() == 2);
	for (auto p : nmm.get_vals("k1"))
	{
		TEST_CHECK(p == "V1" || p == "v1");
	}
	TEST_CHECK(nmm.get_vals("K1").size() == 2);
	for (auto p : nmm.get_vals("K1"))
	{
		TEST_CHECK(p == "V1" || p == "v1");
	}
	nmm.clear();
	TEST_CHECK(nmm.size() == 0);
}
