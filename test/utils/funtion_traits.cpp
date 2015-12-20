#include "UnitTest.hpp"
#include <cinatra/utils/function_traits.hpp>

TEST_CASE(function_traits_test)
{
    TEST_CHECK(typeid(function_traits<int()>::return_type) == typeid(int));
    TEST_CHECK(typeid(function_traits<int(char)>::function_type) == typeid(int(char)));
    TEST_CHECK(function_traits<void(int, char)>::arity == 2);
    TEST_CHECK(typeid(function_traits<void(int, char)>::args<0>::type) == typeid(int));
    TEST_CHECK(typeid(function_traits<void(int, char)>::args<1>::type) == typeid(char));
}
