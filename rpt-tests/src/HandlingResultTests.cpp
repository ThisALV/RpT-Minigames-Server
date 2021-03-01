#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Utils/HandlingResult.hpp>


using namespace RpT::Utils;


BOOST_AUTO_TEST_SUITE(HandlingResultTests)

BOOST_AUTO_TEST_CASE(DefaultConstructor) {
    const HandlingResult result;

    BOOST_CHECK(result); // Must be true, as there isn't any error
    BOOST_CHECK_THROW(result.errorMessage(), NoErrorMessage); // No error, so trying to get message should throws
}

BOOST_AUTO_TEST_CASE(ErrorMessageConstructor) {
    const HandlingResult result { "An error" };

    BOOST_CHECK(!result); // Must be false, as there is reported error
    BOOST_CHECK_EQUAL(result.errorMessage(), "An error"); // Error should be the one given to constructor
}

BOOST_AUTO_TEST_SUITE_END()