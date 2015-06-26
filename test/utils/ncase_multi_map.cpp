#include <boost/test/unit_test.hpp>
#include <cinatra/utils.hpp>

#include <iostream>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(ncase_multi_map_test)
{
	NcaseMultiMap nmm;
	nmm.add("k1", "v1");
	nmm.add("K1", "V1");
	BOOST_CHECK(nmm.get_count("k1") == 2);
	BOOST_CHECK(nmm.get_count("K1") == 2);
	BOOST_CHECK(nmm.get_val("k1") == "v1");
	BOOST_CHECK(nmm.get_val("K1") == "v1");
	BOOST_CHECK(nmm.val_ncase_equal("K1","V1"));
	BOOST_CHECK(nmm.get_vals("k1").size() == 2);
	for (auto p : nmm.get_vals("k1"))
	{
		BOOST_CHECK(p == "V1" || p == "v1");
	}
	BOOST_CHECK(nmm.get_vals("K1").size() == 2);
	for (auto p : nmm.get_vals("K1"))
	{
		BOOST_CHECK(p == "V1" || p == "v1");
	}
	nmm.clear();
	BOOST_CHECK(nmm.size() == 0);
}
