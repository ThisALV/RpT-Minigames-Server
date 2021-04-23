#ifndef RPTOGETHER_SERVER_TEXTPROTOCOLPARSER_HPP
#define RPTOGETHER_SERVER_TEXTPROTOCOLPARSER_HPP

#include <stdexcept>
#include <string_view>
#include <vector>


namespace RpT::Utils {


/**
 * @brief Thrown by `TextProtocolParser` constructor if given number of parsed words hasn't been reached
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NotEnoughWords : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message
     *
     * @param actual_count Actual words count
     * @param expected Expected words count
     */
    NotEnoughWords(const unsigned int actual_count, const unsigned int expected) : std::logic_error {
        "Expected " + std::to_string(expected) + " words, but only got " + std::to_string(actual_count)
    } {}
};


/**
 * @brief Thrown by `TextProtocolParser::getParsedWord()` if given index is out of range and doesn't correspond to
 * any parsed word.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ParsedIndexOutOfRange : std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message
     *
     * @param i Caller asked word index
     * @param count Actual number of parsed words
     */
    ParsedIndexOutOfRange(const std::size_t i, const std::size_t count) : std::logic_error {
        "Tried to get parsed word " + std::to_string(i) + " but only has " + std::to_string(count)
    } {}
};


/**
 * @brief Parser for text-based communication protocols
 *
 * Parser base class only responsibility is to split words into substrings without copying any chars data. It is
 * expected to be inherited by subclass which will offer access to words by providing an interface corresponding with
 * the subclass protocol.
 *
 * For example, a protocol which takes a command and a string will try to parse one word, give access to that
 * specific command (the first word) and give access to argument (unparsed string words).
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class TextProtocolParser {
private:
    std::vector<std::string_view> parsed_words_;
    std::string_view unparsed_words_;

protected:
    /**
     * @brief Constructs parser which will try to parse given words count. Remaining will stay unparsed.
     *
     * Sequences of multiple words separators are treated as one unique merged words separator.
     * Separator at string begin is trimmed, exactly like the ones before and after parsed words.
     * Separator between and after unparsed words will NOT be trimmed.
     *
     * @param protocol_command Protocol text command to parse
     * @param expected_words Number of words to parse, and minimum words count expected inside command
     *
     * @throws NotEnoughWords if actual words count is below given expected number of words
     */
    TextProtocolParser(std::string_view protocol_command, unsigned int expected_words);

    /**
     * @brief Retrieve parsed word at given index
     *
     * @param i Parsed word index, retrieving (i+1)th word
     *
     * @returns Parsed word at given index
     *
     * @throws ParsedIndexOutOfRange if index is superior or equal to current words count
     */
    std::string_view getParsedWord(std::size_t i) const;

    /**
     * @brief Retrieve unparsed substring
     *
     * @return All unparsed substring
     */
    std::string_view unparsedWords() const;

public:
    /// Required for polymorphic instance destruction
    virtual ~TextProtocolParser() = default;
};


}


#endif //RPTOGETHER_SERVER_TEXTPROTOCOLPARSER_HPP
