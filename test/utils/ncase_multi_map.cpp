#include <boost/test/unit_test.hpp>
#include <cinatra/utils.hpp>

#include <iostream>

using namespace cinatra;

/**
 * \FIXME: FAILED
 */ 
BOOST_AUTO_TEST_CASE(ncase_multi_map_test)
{
	NcaseMultiMap nmm;
	nmm.add("k1", "v1");
	nmm.add("K1", "V1");
	BOOST_CHECK(nmm.get_count("k1") == 1);
	BOOST_CHECK(nmm.get_count("K1") == 1);
	BOOST_CHECK(nmm.get_val("k1") == "V1");
	nmm.clear();
	BOOST_CHECK(nmm.size() == 0);
}
