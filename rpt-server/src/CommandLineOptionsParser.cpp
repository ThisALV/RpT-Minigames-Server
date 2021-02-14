#include <RpT-Server/CommandLineOptionsParser.hpp>

#include <algorithm>
#include <cassert>

namespace RpT::Server {

constexpr bool CommandLineOptionsParser::isCommandLineOption(const std::string_view argument) {
    return argument.length() >= PREFIX_LENGTH && argument.substr(0, PREFIX_LENGTH) == OPTION_PREFIX;
}

CommandLineOptionsParser::CommandLineOptionsParser(const int argc, const char** argv,
                                                   const std::initializer_list<std::string_view> allowed_options) {

    bool parsing_option { false }; // If last parsed argument was an option

    // Constants iterators for option checking
    const auto allowed_begin { allowed_options.begin() };
    const auto allowed_end { allowed_options.end() };

    for (int i_arg { 1 }; i_arg < argc; i_arg++) { // Arg index begins at 1 because index 0 is reserved for command name
        const std::string_view arg { argv[i_arg] }; // Currently parsed single argument

        if (isCommandLineOption(arg)) {
            if (std::find(allowed_begin, allowed_end, arg) == allowed_end) { // Check if option is found in allowed list
                // If not, arguments are invalids

                const std::string option_name { arg }; // Necessary to use concatenation on value
                throw InvalidCommandLineOptions { "Option \"" + option_name + "\" isn't allowed" };
            }

            // Add option to parsed options
            const auto insert_result { parsed_options_.insert({ std::string { arg }, {} }) };

            // Check if the insertion was successful or not
            if (!insert_result.second) {
                // If it failed, it is because the option has already been inputted

                const std::string option { arg }; // Necessary to use concatenation on option name
                throw InvalidCommandLineOptions { "Option \"" + option + "\" used at least twice" };
            }

            parsing_option = true;
        } else {
            // If argument isn't an option, it is a value. And if it is a value, it must be preceded by an option
            if (!parsing_option) {
                const std::string value { arg }; // Necessary to use concatenation on value
                throw InvalidCommandLineOptions { "Value \"" + value + "\" assigned without any option" };
            }

            // As parsing_option cannot be true at 1st iteration, parsing_option should be false if i_arg == 1
            assert(i_arg > 1);

            // Get assigned option name, which corresponds to the previous argument
            const std::string& option_name { argv[i_arg - 1] };
            parsed_options_.at(option_name) = arg;
        }
    }
}

bool CommandLineOptionsParser::has(std::string_view option) const {
    return parsed_options_.count(option) == 1;
}

std::string_view CommandLineOptionsParser::get(std::string_view option) const {
    const std::string option_name { option }; // Necessary for exceptions message construction

    if (!has(option)) // Check if option exists
        throw UnknownOption { option_name };

    try {
        return parsed_options_.at(option).value(); // Try to get option value
    } catch (const std::bad_optional_access&) { // If no value is assigned, bad_optional_access will be thrown
        throw NoValueAssigned { option_name }; // Catching error to throw our own exception
    }
}

}
