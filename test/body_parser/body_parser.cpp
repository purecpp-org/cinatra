#include "UnitTest.hpp"
#include <cinatra/body_parser.hpp>

using namespace cinatra;

TEST_CASE(urlencoded_body_parser_test)
{
    CaseMap cm = urlencoded_body_parser("k%261=v%3d1&k%262=v%3d2");
    TEST_CHECK(cm["k&1"] == "v=1");
    TEST_CHECK(cm["k&2"] == "v=2");
}
