#include <boost/test/unit_test.hpp>
#include <cinatra/router.hpp>

using namespace cinatra;

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
		Request req;
		req.method = "GET";
		req.path = "/test";
		Response res;
		BOOST_CHECK(r.handle(req, res));
		BOOST_CHECK(flag);
		/**
 		 * \TODO：
		 *	flag = false;
		 *	req.path = "/test/tt";
		 *	BOOST_CHECK(r.handle(req, res));
		 *	BOOST_CHECK(flag);
		 */	
	}
};
}

BOOST_AUTO_TEST_CASE(router_basic_test)
{
	Cinatra::router_basic_test();
}
