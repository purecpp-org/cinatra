#include "UnitTest.hpp"
#include <cinatra/context_container.hpp>
#include <string>

using namespace cinatra;

TEST_CASE(context_container_req_ctx_test)
{
    app_ctx_container_t acc;
    ContextContainer cc{acc};
    cc.add_req_ctx<int>(1);
    cc.add_req_ctx<double>(3.1);
    struct IntCtx { using Context = int; };
    struct DoubleCtx { using Context = double; };
    TEST_REQUIRE(cc.has_req_ctx<IntCtx>());
    TEST_REQUIRE(cc.has_req_ctx<DoubleCtx>());
    TEST_CHECK(cc.get_req_ctx<IntCtx>() == 1);    
    TEST_CHECK(cc.get_req_ctx<DoubleCtx>() == 3.1);    
}

TEST_CASE(context_container_app_ctx_test)
{
    app_ctx_container_t acc;
    ContextContainer cc{acc};
    cc.set_app_ctx<char>("char", 'c');
    cc.set_app_ctx<std::string>("string", "string");
    TEST_REQUIRE(cc.has_app_ctx("char"));
    TEST_REQUIRE(cc.has_app_ctx("string"));
    std::string ch = "char";
    TEST_CHECK(cc.get_app_ctx<char>(ch));
    TEST_CHECK(cc.get_app_ctx<std::string>("string") == "string");
}
