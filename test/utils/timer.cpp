#include <boost/test/unit_test.hpp>
#include <cinatra/utils/utils.hpp>
#include <thread>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(timer_test)
{
    Timer timer;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    BOOST_CHECK(timer.elapsed() >= 1000 && timer.elapsed() < 2000);                         /**< milliseconds */
    BOOST_CHECK(timer.elapsed_second() == 1);                                               /**< seconds */
    BOOST_CHECK(timer.elapsed_micro() >= 1000000 && timer.elapsed_micro() < 2000000);       /**< microseconds */
    BOOST_CHECK(timer.elapsed_nano() >= 1000000000 && timer.elapsed_nano() < 2000000000);   /**< nanoseconds */
    BOOST_CHECK(timer.elapsed_minutes() == 0);   /**< minutes */
    BOOST_CHECK(timer.elapsed_hours() == 0);   /**< hours */
}
