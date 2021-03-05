#include <RpT-Utils/TextProtocolParser.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Utils {


TextProtocolParser::TextProtocolParser(const std::string_view protocol_command,
                                       const unsigned int expected_words) {

    // Begin and end constant iterators for SR command string
    const auto cmd_begin { protocol_command.cbegin() };
    const auto cmd_end { protocol_command.cend() };

    // Starts string parsing from the first word
    auto word_begin { cmd_begin };
    auto word_end { std::find(cmd_begin, cmd_end, ' ') };

    // While entire string hasn't be parsed (while word begin isn't at string end) and required words count hasn't
    // been reached
    while (parsed_words_.size() != expected_words && word_begin != word_end) {
        // Calculate index for word begin, and next word size
        const auto word_begin_i { word_begin - cmd_begin };
        const auto word_length { word_end - word_begin };

        // Add currently parsed word to words
        parsed_words_.push_back(protocol_command.substr(word_begin_i, word_length ));

        word_begin = word_end; // Parsing goes to next word

        if (word_end != cmd_end) { // Is the string end reached ?
            word_begin++; // Then, move word begin after previously found space
            word_end = std::find(word_begin, cmd_end, ' '); // And find next word end starting from next word begin
        }

        assert(word_begin == cmd_end || *word_begin != ' '); // Word begin must be alphanum char (1st word char)
        assert(word_end == cmd_end || *word_end == ' '); // Word end must be space or command end (last word char + 1)
        assert(std::find(word_begin, word_end, ' ') == cmd_end); // Current word must NOT contain any space char
    }

    // Where the next word should have begin is unparsed partition begin
    const auto unparsed_words_begin_i { word_begin - cmd_begin };
    unparsed_words_ = protocol_command.substr(unparsed_words_begin_i);
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
