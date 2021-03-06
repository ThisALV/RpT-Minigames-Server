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


BOOST_AUTO_TEST_CASE(EmptyCommandExpectedZeroWords) {

}

BOOST_AUTO_TEST_CASE(EmptyCommandExpectedOneWord) {

}

BOOST_AUTO_TEST_CASE(ManySpacesOnlyExpectedOneWord) {

}

BOOST_AUTO_TEST_CASE(ManySpacesOnlyExpectedZeroWord) {

}

BOOST_AUTO_TEST_CASE(ManySpacesOnlyExpectedTwoWords) {

}




BOOST_AUTO_TEST_SUITE_END()
