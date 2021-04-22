#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Core/ServiceContext.hpp>


using namespace RpT::Core;


BOOST_AUTO_TEST_SUITE(ServiceContextTests)

BOOST_AUTO_TEST_CASE(DefaultConstructed) {
    ServiceContext context;

    BOOST_CHECK_EQUAL(context.newEventPushed(), 0);
    BOOST_CHECK_EQUAL(context.newEventPushed(), 1);
    BOOST_CHECK_EQUAL(context.newEventPushed(), 2);

    BOOST_CHECK_EQUAL(context.newTimerCreated(), 0);
    BOOST_CHECK_EQUAL(context.newTimerCreated(), 1);
    BOOST_CHECK_EQUAL(context.newTimerCreated(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
