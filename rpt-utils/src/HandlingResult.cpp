#include <RpT-Utils/HandlingResult.hpp>


namespace RpT::Utils {


HandlingResult::HandlingResult(std::string error_message) : possible_error_message_ { std::move(error_message) } {}

HandlingResult::operator bool() const {
    return !possible_error_message_.has_value();
}

const std::string& HandlingResult::errorMessage() const {
    if (!possible_error_message_.has_value()) // An error must have occurred
        throw NoErrorMessage {};

    return *possible_error_message_;
}


}
