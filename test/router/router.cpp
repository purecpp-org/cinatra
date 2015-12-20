#include "UnitTest.hpp"
#include <cinatra/http_router.hpp>

#include <iostream>

using namespace cinatra;

TEST_CASE(router_dispatch_test_0)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/1", [&](Request&, Response&) {
		flag = true;
	});
	Request req{ "/1", {}, "GET", "/1", {}, {}};
	Response res;
	app_ctx_container_t container;
	ContextContainer ctx(container);
	r.dispatch(req, res, ctx);
	TEST_CHECK(flag);
}

TEST_CASE(router_dispatch_test_1)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/:id", [&](Request&, Response&, int a) {
		flag = true;
		TEST_CHECK(a == 1);
	});
	Request req{ "/1", {}, "GET", "/1", {}, {}};
	Response res;
	app_ctx_container_t container;
	ContextContainer ctx(container);
	r.dispatch(req, res, ctx);
	TEST_CHECK(flag);
}

TEST_CASE(router_dispatch_test_2)
{
	bool flag = false;
	HTTPRouter r;
	r.route("/1/2/", [&](Request&, Response&) {
		flag = true;
	});
	r.route("/1", [](Request&, Response&) {
		TEST_CHECK(false);
	});
	Request req{ "/1/2", {}, "GET", "/1/2", {}, {}};
	Response res;
	app_ctx_container_t container;
	ContextContainer ctx(container);
	r.dispatch(req, res, ctx);
	TEST_CHECK(flag);
}

TEST_CASE(router_funcname_test)
{
	HTTPRouter r;
	TEST_CHECK(r.getFuncName("/hello/world/:name/:age") == "/hello/world");
}
