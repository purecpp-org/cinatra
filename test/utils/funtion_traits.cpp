#include <boost/test/unit_test.hpp>
#include <cinatra/utils/function_traits.hpp>

BOOST_AUTO_TEST_CASE(function_traits_test)
{
    BOOST_CHECK(typeid(function_traits<int()>::return_type) == typeid(int));
    BOOST_CHECK(typeid(function_traits<int(char)>::function_type) == typeid(int(char)));
    BOOST_CHECK(function_traits<void(int, char)>::arity == 2);
    BOOST_CHECK(typeid(function_traits<void(int, char)>::args<0>::type) == typeid(int));
    BOOST_CHECK(typeid(function_traits<void(int, char)>::args<1>::type) == typeid(char));
}
