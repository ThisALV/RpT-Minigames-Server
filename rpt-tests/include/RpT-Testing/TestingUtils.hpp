#ifndef RPTOGETHER_SERVER_TESTINGUTILS_HPP
#define RPTOGETHER_SERVER_TESTINGUTILS_HPP

#include <boost/test/unit_test.hpp>

#include <optional>


/**
 * @brief Utilities functions for RpT testing
 */
namespace RpT::Testing {


/**
 * @brief Make comparison and error printing for optional values
 *
 * @tparam T Type to test for if optional value is initialized
 *
 * @param lhs Left arg to compare
 * @param rhs Right arg to compare
 */
template<typename T> void boostCheckOptionalsEqual(std::optional<T>&& lhs, std::optional<T>&& rhs) {
    const bool l_has_value { lhs.has_value() };
    const bool r_has_value { rhs.has_value() };

    // Both optional must have the same state to be equals
    BOOST_CHECK_EQUAL(l_has_value, r_has_value);

    // And if they both have value...
    if (l_has_value && r_has_value) {
        // ...these values must be equals.
        BOOST_CHECK_EQUAL(*lhs, *rhs);
    }
}


}


#endif //RPTOGETHER_SERVER_TESTINGUTILS_HPP
