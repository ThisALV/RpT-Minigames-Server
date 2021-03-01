#ifndef RPTOGETHER_SERVER_HANDLINGRESULT_HPP
#define RPTOGETHER_SERVER_HANDLINGRESULT_HPP

#include <optional>
#include <stdexcept>
#include <string>

/**
 * @file HandlingResult.hpp
 */


namespace RpT::Utils {


/**
 * @brief Thrown by `HandlingResult::errorMessage()` if `handler` is true
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NoErrorMessage : public std::logic_error {
public:
    /**
     * @brief Constructs exception with basic error message
     */
    explicit NoErrorMessage() : std::logic_error { "No error message available, handler completed successfully" } {}
};


/**
 * @brief Provides information about handler execution errors, if any occurred
 *
 * Allows to know if handler was done successfully, and if not, what happened during execution
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class HandlingResult {
private:
    std::optional<std::string> possible_error_message_;

public:
    /**
     * @brief Handling was done successfully, no errors
     */
    HandlingResult() = default;

    /**
     * @brief Error occurred during handler exec
     *
     * @param error_message Message describing what king of error happened
     */
    explicit HandlingResult(std::string error_message);

    /**
     * @brief Has the handler completed successfully ?
     *
     * @returns `true` if handler completed success, `false` otherwise
     */
    operator bool() const;

    /**
     * @brief Gets error which happened during handler execution
     *
     * @returns Error message
     *
     * @throws NoErrorMessage if handler actually completed successfully
     */
    const std::string& errorMessage() const;
};


}


#endif //RPTOGETHER_SERVER_HANDLINGRESULT_HPP
