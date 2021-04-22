#ifndef RPTOGETHER_SERVER_COMMANDLINEOPTIONSPARSER_HPP
#define RPTOGETHER_SERVER_COMMANDLINEOPTIONSPARSER_HPP

#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * @file CommandLineOptionsParser.hpp
 */


/**
 * @brief RpT-Server utilities provided to user, and useful among all server components and libraries
 */
namespace RpT::Utils {

/**
 * @brief Base class for exceptions that can occur during arguments parsing or options manipulation
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class OptionsError : public std::logic_error {
public:
    /**
     * @brief Constructs error with custom error message provided by caller
     *
     * @param reason Reason for command-line options to be ill-formed
     */
    explicit OptionsError(const std::string& reason) : std::logic_error { reason } {}
};

/**
 * @brief Thrown by `CommandLineOptionsParser` constructor if arguments have invalid format
 *
 * @see CommandLineOptionsParser
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InvalidCommandLineOptions : public OptionsError {
public:
    /**
     * @brief Constructs exception with given reason
     *
     * @param reason Why arguments format isn't valid
     */
    explicit InvalidCommandLineOptions(const std::string& reason) : OptionsError { reason } {}
};

/**
 * @brief Thrown by `CommandLineOptionsParser::get()` if option doesn't exists
 *
 * @see CommandLineOptionsParser
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class MissingOption : public OptionsError {
public:
    /**
     * @brief Constructs exception with error message
     *
     * @param option Option that was look up
     */
    explicit MissingOption(const std::string& option) : OptionsError { "Option \"" + option + "\" is missing" } {}
};

/**
 * @brief Thrown by `CommandLineOptionsParser::get()` if option exists but hasn't any assigned value
 *
 * @see CommandLineOptionsParser
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NoValueAssigned : public OptionsError {
public:
    /**
     * @brief Constructs exception with error message
     *
     * @param option Option which hasn't any assigned value
     */
    explicit NoValueAssigned(const std::string& option)
    : OptionsError { "Option \"" + option + "\" hasn't any assigned value" } {}
};

/**
 * @brief `main()` command line arguments parser
 *
 * See constructor doc for details about expected options format.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class CommandLineOptionsParser {
private:
    static constexpr std::string_view OPTION_PREFIX { "--" };
    static constexpr std::size_t PREFIX_LENGTH { OPTION_PREFIX.length() };

    std::unordered_map<std::string_view, std::optional<std::string_view>> parsed_options_;

    /**
    * @brief Returns if given command line single argument is option or not
    *
    * @param argument Argument to parse
    *
    * @return `true` if parsed argument is prefixed by `--`, else it returns `false`
    */
    static constexpr bool isCommandLineOption(std::string_view argument);

    /**
     * @brief Returns option name for a given option argument
     *
     * For option argument "--delay", returns option name "delay".
     *
     * @param arg Option argument on which option name will be extracted
     *
     * @returns Option name for option argument
     */
    static constexpr std::string_view optionNameOf(std::string_view arg);

public:
    /**
     * @brief Parse command-line options
     *
     * Each command-line arg is parsed. If it begins with `--` prefix, it is considered as an option. Else, it is
     * considered as a value.
     *
     * An option might be followed by a value.
     * A parsed option must be included in allowed_options. If not, `InvalidCommandLineOptions` is thrown.
     * A parsed value must follow an option argument to be assigned at. If not, `InvalidCommandLineOptions` is thrown.
     *
     * @param argc Number of arguments to parse
     * @param argv Arguments to parse
     * @param allowed_options List of recognized option names
     *
     * @note As `CommandLineOptionsParser` works with string_view, original argv array must NOT be modified.
     *
     * @throws InvalidCommandLineOptions If arguments format is invalid
     */
    CommandLineOptionsParser(int argc, const char** argv, std::initializer_list<std::string_view> allowed_options);

    /**
     * @brief Returns if option has been parsed
     *
     * @param option Name of option to look up for
     *
     * @returns If `option` has been parsed
     */
    bool has(std::string_view option) const;

    /**
     *
     * @param option Name of option that will return the value
     *
     * @throws MissingOption If `option` doesn't exists
     * @throws NoValueAssigned If `option` exists but hasn't any assigned value
     *
     * @return Value of `option`
     */
    std::string_view get(std::string_view option) const;
};


}

#endif //RPTOGETHER_SERVER_COMMANDLINEOPTIONSPARSER_HPP
