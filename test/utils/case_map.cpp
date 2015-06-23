#include <boost/test/unit_test.hpp>
#include <cinatra/utils.hpp>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(case_map_test)
{
	CaseMap cm;
	cm.add("k1", "v1");
	BOOST_CHECK(cm.has_key("k1"));
	BOOST_CHECK(cm.get_val("k1") == "v1");
	cm.add("K1", "V1");
	BOOST_CHECK(cm.get_val("k1") == "v1");
	auto all = cm.get_all();
	BOOST_CHECK(all.size() == 2);
	for(auto p : all) {
		if(p.first == "k1") {
			BOOST_CHECK(p.second == "v1");
		} else if(p.first == "K1") {
			BOOST_CHECK(p.second == "V1");
		} else {
			BOOST_CHECK(false);	/**< 不应该有其他值 */
		}
	}
	cm.clear();
	BOOST_CHECK(cm.size() == 0);
}
