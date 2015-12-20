#include "UnitTest.hpp"
#include <cinatra/utils/utils.hpp>

using namespace cinatra;

TEST_CASE(case_map_test)
{
	CaseMap cm;
	cm.add("k1", "v1");
	TEST_CHECK(cm.has_key("k1"));
	TEST_CHECK(cm.get_val("k1") == "v1");
	cm.add("K1", "V1");
	TEST_CHECK(cm.get_val("k1") == "v1");
	auto all = cm.get_all();
	TEST_CHECK(all.size() == 2);
	for(auto p : all) {
		if(p.first == "k1") {
			TEST_CHECK(p.second == "v1");
		} else if(p.first == "K1") {
			TEST_CHECK(p.second == "V1");
		} else {
			TEST_CHECK(false);	/**< 不应该有其他值 */
		}
	}
	cm.clear();
	TEST_CHECK(cm.size() == 0);
}
