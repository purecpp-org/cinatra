#include <boost/test/unit_test.hpp>
#include <cinatra/context_container.hpp>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(context_container_test)
{
    app_ctx_container_t acc;
    ContextContainer cc{acc};
    cc.add_req_ctx<int>(1);
    cc.add_req_ctx<double>(3.1);
    struct IntCtx { using Context = int; };
    struct DoubleCtx { using Context = double; };
    BOOST_REQUIRE(cc.has_req_ctx<IntCtx>());
    BOOST_REQUIRE(cc.has_req_ctx<DoubleCtx>());
    BOOST_CHECK(cc.get_req_ctx<IntCtx>() == 1);    
    BOOST_CHECK(cc.get_req_ctx<DoubleCtx>() == 3.1);    
}
