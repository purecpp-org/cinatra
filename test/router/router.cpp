#include <boost/test/unit_test.hpp>
#include <cinatra/http_router.hpp>

#include <iostream>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(router_dispatch_test_0)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/1", [&](Request&, Response&) {
		flag = true;
	});
	Request req{ "/1", {}, "GET", "/1", {}, {}};
	Response res;
	r.dispatch(req, res);
	BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(router_dispatch_test_1)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/:id", [&](Request&, Response&, int a) {
		flag = true;
		BOOST_CHECK(a == 1);
	});
	Request req{ "/1", {}, "GET", "/1", {}, {}};
	Response res;
	r.dispatch(req, res);
	BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(router_dispatch_test_2)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/1/2/", [&](Request&, Response&) {
		flag = true;
	});
	r.route("/1", [](Request&, Response&) {
		BOOST_CHECK(false);
	});
	Request req{ "/1/2", {}, "GET", "/1/2", {}, {}};
	Response res;
	r.dispatch(req, res);
	BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(router_funcname_test)
{
	HTTPRouter r;
	BOOST_CHECK(r.getFuncName("/hello/world/:name/:age") == "/hello/world");
}
