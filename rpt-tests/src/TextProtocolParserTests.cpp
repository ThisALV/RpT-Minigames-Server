#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Utils/TextProtocolParser.hpp>


using namespace RpT::Utils;


/**
 * @brief Basic implementation for `RpT::Core::TextProtocolParser`, only gives direct access to parsed words and
 * unparsed substring
 */
class SimpleParser : public TextProtocolParser {
public:
    SimpleParser(const std::string_view protocol_command, const unsigned int expected_words) :
    TextProtocolParser { protocol_command, expected_words } {}

    std::string_view wordAt(const std::size_t i) const {
        return getParsedWord(i);
    }

    std::string_view unparsed() const {
        return unparsedWords();
    }
};


BOOST_AUTO_TEST_SUITE(TextProtocolParserTests)

/*
 * Empty command tests
 */

BOOST_AUTO_TEST_SUITE(EmptyCommand)

// Trimmed empty command tests
BOOST_AUTO_TEST_SUITE(TrimmedCommand)

BOOST_AUTO_TEST_CASE(ExpectedZeroWords) {
    const SimpleParser parser { "", 0 };

    // As no word has been parsed, trying to retrieve the first one should throws an error
    BOOST_CHECK_THROW(parser.wordAt(0), ParsedIndexOutOfRange);
    // And as command is empty, nothing remains unparsed
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedOneWord) {
    // One word is expected, so empty given command should throws an error
    BOOST_CHECK_THROW((SimpleParser { "", 1 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

// Non-trimmed empty command tests
BOOST_AUTO_TEST_SUITE(NonTrimmedCommand)

BOOST_AUTO_TEST_CASE(ExpectedZeroWords) {
    const SimpleParser parser { "    ", 0 };

    // As no word has been parsed, trying to retrieve the first one should throws an error
    BOOST_CHECK_THROW(parser.wordAt(0), ParsedIndexOutOfRange);
    // And as command begin is trimmed from parsing, nothing should remain unparsed
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedOneWord) {
    // One word is expected, so empty given command should throws an error
    BOOST_CHECK_THROW((SimpleParser { "    ", 1 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

/*
 * Single word tests
 */

BOOST_AUTO_TEST_SUITE(SingleWord)

// Trimmed word tests

BOOST_AUTO_TEST_SUITE(TrimmedWord)

BOOST_AUTO_TEST_CASE(ExpectedZeroWords) {
    const SimpleParser parser { "Command", 0 };

    // As no word has been parsed (because no word was expected), trying to retrieve the first one should throws error
    BOOST_CHECK_THROW(parser.wordAt(0), ParsedIndexOutOfRange);
    // As parsing was immediately stopped, whole command should remain unparsed
    BOOST_CHECK_EQUAL(parser.unparsed(), "Command");
}

BOOST_AUTO_TEST_CASE(ExpectedOneWord) {
    const SimpleParser parser { "Command", 1 };

    // As a word was expected, single word should be parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    // But only one word should have been parsed
    BOOST_CHECK_THROW(parser.wordAt(1), ParsedIndexOutOfRange);
    // As single trimmed word has been parsed, nothing should remain unparsed
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedTwoWords) {
    // Two words expected, but only one found
    BOOST_CHECK_THROW((SimpleParser { "Command", 2 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

// Non-trimmed word tests

BOOST_AUTO_TEST_SUITE(NonTrimmedWord)

BOOST_AUTO_TEST_CASE(ExpectedZeroWords) {
    const SimpleParser parser { "  Command   ", 0 };

    // As no word has been parsed (because no word was expected), trying to retrieve the first one should throws error
    BOOST_CHECK_THROW(parser.wordAt(0), ParsedIndexOutOfRange);
    // As parsing was immediately stopped but command begin trimmed, whole command since first unparsed word should
    // remain unparsed
    BOOST_CHECK_EQUAL(parser.unparsed(), "Command   ");
}

BOOST_AUTO_TEST_CASE(ExpectedOneWord) {
    const SimpleParser parser { "  Command   ", 1 };

    // As a word was expected, single trimmed word should be parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    // But only one word should have been parsed
    BOOST_CHECK_THROW(parser.wordAt(1), ParsedIndexOutOfRange);
    // As single trimmed word has been parsed, nothing should remain unparsed because command begin is trimmed and so
    // are separators after parsed words
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedTwoWords) {
    // Two words expected, but only one found
    BOOST_CHECK_THROW((SimpleParser { "Command", 2 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

/*
 * Three words tests
 */

BOOST_AUTO_TEST_SUITE(ThreeWords)

// Trimmed command, single separator between words
BOOST_AUTO_TEST_SUITE(TrimmedCommand)

BOOST_AUTO_TEST_CASE(ExpectedTwoWords) {
    const SimpleParser parser { "Command Arg1 Arg2", 2 };

    // Both first and second trimmed words should have been parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    BOOST_CHECK_EQUAL(parser.wordAt(1), "Arg1");

    // Third word should NOT have been parsed, but second argument end-separators should have been trimmed
    BOOST_CHECK_THROW(parser.wordAt(2), ParsedIndexOutOfRange);
    BOOST_CHECK_EQUAL(parser.unparsed(), "Arg2");
}

BOOST_AUTO_TEST_CASE(ExpectedThreeWords) {
    const SimpleParser parser { "Command Arg1 Arg2", 2 };

    // All trimmed words should have been parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    BOOST_CHECK_EQUAL(parser.wordAt(1), "Arg1");
    BOOST_CHECK_EQUAL(parser.wordAt(2), "Arg2");
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedFiveWords) {
    // Five words expected, but only three found
    BOOST_CHECK_THROW((SimpleParser { "Command Arg1 Arg2", 5 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

// Non-trimmed command, sometimes many separators between words
BOOST_AUTO_TEST_SUITE(NonTrimmedCommand)

BOOST_AUTO_TEST_CASE(ExpectedTwoWords) {
    const SimpleParser parser { "  Command   Arg1  Arg2   ", 2 };

    // Both first and second trimmed words should have been parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    BOOST_CHECK_EQUAL(parser.wordAt(1), "Arg1");

    // Third word should NOT have been parsed, but second argument end-separators should have been trimmed
    BOOST_CHECK_THROW(parser.wordAt(2), ParsedIndexOutOfRange);
    BOOST_CHECK_EQUAL(parser.unparsed(), "Arg2   ");
}

BOOST_AUTO_TEST_CASE(ExpectedThreeWords) {
    const SimpleParser parser { "  Command   Arg1  Arg2   ", 2 };

    // All trimmed words should have been parsed
    BOOST_CHECK_EQUAL(parser.wordAt(0), "Command");
    BOOST_CHECK_EQUAL(parser.wordAt(1), "Arg1");
    BOOST_CHECK_EQUAL(parser.wordAt(2), "Arg2");
    BOOST_CHECK(parser.unparsed().empty());
}

BOOST_AUTO_TEST_CASE(ExpectedFiveWords) {
    // Five words expected, but only three found
    BOOST_CHECK_THROW((SimpleParser { "  Command   Arg1  Arg2   ", 5 }), NotEnoughWords);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
