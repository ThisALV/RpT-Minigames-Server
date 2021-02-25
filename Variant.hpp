#ifndef RPTOGETHER_SERVER_VARIANT_HPP
#define RPTOGETHER_SERVER_VARIANT_HPP

#include <functional>
#include <type_traits>
#include <utility>

/**
 * @file Variant.hpp
 *
 * @brief Configuration header required to use working variant library by current build compiler
 */


/// Boost.Variant library
#define RPT_VARIANT_BOOST 1
/// Standard variant library
#define RPT_VARIANT_STANDARD 2

/**
 * @brief <variant> library working for current build compiler
 *
 * Variant library support status, must be RPT_VARIANT_BOOST (0) or RPT_VARIANT_STANDARD (1).
 *
 * Available as a macro to allow conditional includes and using directives.
 */
#define RPT_VARIANT_SUPPORT @RPT_VARIANT_SUPPORT@


// Includes correct library header and sets variant template alias depending on RpT variant support

#if RPT_VARIANT_SUPPORT == RPT_VARIANT_BOOST

#include <boost/variant.hpp>

namespace supported_variants {

/**
 * @brief Cross-compilers supported Variant template implementation
 */
template<typename... Ts> using variant = boost::variant<Ts...>;

}

#elif RPT_VARIANT_SUPPORT == RPT_VARIANT_STANDARD

#include <variant>

namespace supported_variants {

/**
 * @brief Cross-compilers supported Variant template implementation
 */
template<typename... Ts> using variant = std::variant<Ts...>;

}

#endif


/**
 * @brief Wrapper utilities to access variant library working for current compiler
 *
 * If compiler is Clang 6, then Boost.Variant will be used.
 * Else, STL variant library is kept.
 */
namespace supported_variants {

/**
 * @brief Cross-compilers supported visit function template implementation
 *
 * @tparam Visitor Type of handler for type-based visit
 * @tparam Ts Possible visited value types
 *
 * @param visitor Handler for type-based visit
 * @param operand Visited value for handler execution
 *
 * @returns Value returned by given visitor
 */
template<typename Visitor, typename... Ts>
std::invoke_result_t<Visitor, variant<Ts...>> visit(Visitor&& visitor, const variant<Ts...>& operand) {
#if RPT_VARIANT_SUPPORT == RPT_VARIANT_BOOST // If Boost.Variant is used, then boost::apply_visitor must be called
    return boost::apply_visitor(visitor, operand); // Boost visit function doesn't forward reference the visitor
#elif RPT_VARIANT_SUPPORT == RPT_VARIANT_STANDARD // If STL variant is used, then std::visit must be called
    return std::visit(std::forward<Visitor>(visitor), operand); // STL visit DOES forward reference the visitor
#else
    static_assert(false, "Unable to get RpT variant library support");
#endif
}

}

#endif //RPTOGETHER_SERVER_VARIANT_HPP
