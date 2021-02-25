#ifndef RPTOGETHER_SERVER_VARIANT_HPP
#define RPTOGETHER_SERVER_VARIANT_HPP

#include <functional>
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


// Includes correct library header, sets appropriate typedef and define function alias depending on supported variant

#if RPT_VARIANT_SUPPORT == RPT_VARIANT_BOOST

#include <boost/variant.hpp>

/**
 * @brief Cross-compilers supported Variant template implementation
 */
template<typename... Ts> using supported_variant = boost::variant<Ts...>;


#elif RPT_VARIANT_SUPPORT == RPT_VARIANT_STANDARD

#include <variant>

/**
 * @brief Cross-compilers supported Variant template implementation
 */
template<typename... Ts> using supported_variant = std::variant<Ts...>;

#endif

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
std::invoke_result_t<Visitor, supported_variant<Ts...>> supported_visit(Visitor&& visitor,
                                                                        const supported_variant<Ts...>& operand) {

#if RPT_VARIANT_SUPPORT == RPT_VARIANT_BOOST
    return boost::apply_visitor(visitor, operand);
#elif RPT_VARIANT_SUPPORT == RPT_VARIANT_STANDARD
    return std::visit(std::forward<Visitor>(visitor), operand);
#else
    static_assert(false, "Unable to get RpT variant library support");
#endif
}

#endif //RPTOGETHER_SERVER_VARIANT_HPP
