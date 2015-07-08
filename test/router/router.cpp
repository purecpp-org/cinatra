#include <boost/test/unit_test.hpp>
#include <cinatra/router.hpp>
#include <vector>

namespace cinatra {
class Cinatra {
public:
	static void router_basic_test() {
		Router r("/test");
		bool flag = false;
		r.set_handler([&flag](const Request&, Response&) {
			flag = true;
		});
		BOOST_CHECK(!flag);
		Request req("/test", std::vector<char>(), "GET", "/test", CaseMap(), NcaseMultiMap());
		Response res;
		BOOST_CHECK(r.handle(req, res));
		BOOST_CHECK(flag);
		/**
 		 * \TODO：path test
		 */	
	}
};
}

BOOST_AUTO_TEST_CASE(router_basic_test)
{
	cinatra::Cinatra::router_basic_test();
}
