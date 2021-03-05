#include <RpT-Utils/TextProtocolParser.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Utils {


TextProtocolParser::TextProtocolParser(const std::string_view protocol_command,
                                       const unsigned int expected_words) {

    // Iterators to non-trimmed begin and end of command string
    const auto cmd_begin { protocol_command.cbegin() };
    const auto cmd_end { protocol_command.cend() };

    std::ptrdiff_t word_begin_i { 0 }; // Currently parsed word begin index
    std::size_t word_length { 0 }; // Currently parsed word length

    auto char_it { cmd_begin }; // Parsing begins at command begin

    // Iterates over all chars until command string end, or until expected parsed words count has been reached
    while (char_it != cmd_end && parsed_words_.size() < expected_words) {
        const char current_char { *char_it };

        if (current_char == ' ') { // When separator is met
            if (word_length != 0) { // If word is currently parsed, push it into parsed words
                const std::string_view current_parsed_word { protocol_command.substr(word_begin_i, word_length) };

                parsed_words_.push_back(current_parsed_word);

                word_length = 0; // No word is currently into parsing stage
            }
        } else { // If iterator is inside any word
            if (word_length == 0) // If new word enters into parsing stage, new word begin position must be saved
                word_begin_i = char_it - cmd_begin;

            word_length++; // Parsing stage word is one char longer after this iteration
        }

        char_it++; // Parsing goes forward to next char
    }

    // Trims after last parsed word
    while (char_it != cmd_end && *char_it == ' ')
        char_it++;

    const std::ptrdiff_t unparsed_words_begin_i { char_it - cmd_begin }; // Index for unparsed substring begin
    unparsed_words_ = protocol_command.substr(unparsed_words_begin_i); // Push unparsed words into instance
}

std::string_view TextProtocolParser::getParsedWord(std::size_t i) const {
    try { // Tries to retrieve word for given index...
        return parsed_words_.at(i);
    } catch (const std::out_of_range&) { // ...and if index is out of range, throw custom exception class
        throw ParsedIndexOutOfRange { i, parsed_words_.size() };
    }
}

std::string_view TextProtocolParser::unparsedWords() const {
    return unparsed_words_;
}


}
