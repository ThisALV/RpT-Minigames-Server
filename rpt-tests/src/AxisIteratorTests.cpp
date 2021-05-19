#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/AxisIterator.hpp>


using namespace MinigamesServices;


// Required for ADL to detect operator<< for AxisType inside MinigamesServices namespace
namespace MinigamesServices {


// Required by BOOST_CHECK_EQUAL macro usage
std::ostream& operator<<(std::ostream& out, const AxisType axis_type) {
    std::string stringified;

    // Adds stringified flags to axis depending on its enabled directional flags

    if (hasFlagOf(axis_type, AxisType::Up))
        stringified += "Up";
    else if (hasFlagOf(axis_type, AxisType::Down))
        stringified += "Down";

    if (hasFlagOf(axis_type, AxisType::Left))
        stringified += "Left";
    else if (hasFlagOf(axis_type, AxisType::Right))
        stringified += "Right";

    return out << stringified;
}


}


/*
 * Constant shortcuts for squares inside grid
 */

constexpr Square EMPTY { Square::Free };
constexpr Square WHITE { Square::White };
constexpr Square BLACK { Square::Black };


/// Provides a grid of dimensions 5x10 where 1st, 3rd and 5th columns are empty, and 2nd and 4th are respectively
/// white and black
class SampleGridFixture {
public:
    static constexpr std::initializer_list<std::initializer_list<Square>> INITIAL_CONFIGURATION {
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
            { EMPTY, WHITE, EMPTY, BLACK, EMPTY },
    };

    Grid grid;

    SampleGridFixture() : grid { INITIAL_CONFIGURATION } {}
};


BOOST_FIXTURE_TEST_SUITE(AxisIteratorTests, SampleGridFixture)


BOOST_AUTO_TEST_CASE(FromEqualsTo) {
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 3 }, { 2, 3 } }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(FromOutsideGrid) {
    BOOST_CHECK_THROW((AxisIterator { grid, { -99, 3 }, { 2, 3 } }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(ToOutsideGrid) {
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 3 }, { 100, 100 } }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(NoAxisBetween) {
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 3, 4 } }), BadCoordinates);
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 1, 4 } }), BadCoordinates);
    BOOST_CHECK_THROW((AxisIterator { grid, { 1, 5 }, { 2, 1 } }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(ForbiddenAxisBetween) {
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 2, 4 }, AxisIterator::EVERY_DIAGONAL_DIRECTION }),
                      BadCoordinates);
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 4, 2 }, AxisIterator::EVERY_DIAGONAL_DIRECTION }),
                      BadCoordinates);

    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 3, 3 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION }),
                      BadCoordinates);
    BOOST_CHECK_THROW((AxisIterator { grid, { 2, 2 }, { 1, 3 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION }),
                      BadCoordinates);
}

BOOST_AUTO_TEST_CASE(HorizontalAxisBetween) {
    AxisIterator it1 { // 2 columns to the right
        grid, { 2, 1 }, { 2, 3 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION
    };
    AxisIterator it2 { // 2 columns to the left
        grid, { 2, 5 }, { 2, 3 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION
    };

    BOOST_CHECK_EQUAL(it1.direction(), AxisType::Right);
    BOOST_CHECK_EQUAL(it2.direction(), AxisType::Left);

    // 1st iterator can move 2 times before it reaches destination, 4 times before the end
    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -2);
    BOOST_CHECK_EQUAL(it1.moveForward(), WHITE);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -1);
    BOOST_CHECK_EQUAL(it1.moveForward(), EMPTY);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 0);
    BOOST_CHECK_EQUAL(it1.moveForward(), BLACK);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 1);
    BOOST_CHECK_EQUAL(it1.moveForward(), EMPTY);

    BOOST_CHECK(!it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 2);
}

BOOST_AUTO_TEST_CASE(VerticalAxisBetween) {
    AxisIterator it1 { // 3 lines to the bottom
            grid, { 6, 2 }, { 9, 2 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION
    };
    AxisIterator it2 { // 1 line to the top
            grid, { 2, 5 }, { 1, 5 }, AxisIterator::EVERY_ORTHOGONAL_DIRECTION
    };

    BOOST_CHECK_EQUAL(it1.direction(), AxisType::Down);
    BOOST_CHECK_EQUAL(it2.direction(), AxisType::Up);

    // 1st iterator can move 3 times before it reaches destination, 4 times before the end
    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -3);
    BOOST_CHECK_EQUAL(it1.moveForward(), WHITE);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -2);
    BOOST_CHECK_EQUAL(it1.moveForward(), WHITE);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -1);
    BOOST_CHECK_EQUAL(it1.moveForward(), WHITE);

    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 0);
    BOOST_CHECK_EQUAL(it1.moveForward(), WHITE);

    BOOST_CHECK(!it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 1);
}

BOOST_AUTO_TEST_CASE(DiagonalAxisBetween) {
    AxisIterator it1 { // 1 line to the top, 1 column to the right
            grid, { 2, 2 }, { 1, 3 }, AxisIterator::EVERY_DIAGONAL_DIRECTION
    };
    AxisIterator it2 { // 2 lines to the top, 2 columns to the left
            grid, { 5, 5 }, { 3, 3 }, AxisIterator::EVERY_DIAGONAL_DIRECTION
    };
    AxisIterator it3 { // 3 lines to the bottom, 3 columnsto the right
            grid, { 2, 2 }, { 5, 5 }, AxisIterator::EVERY_DIAGONAL_DIRECTION
    };
    AxisIterator it4 { // 1 line to the bottom, 1 column to the left
            grid, { 2, 2 }, { 3, 1 }, AxisIterator::EVERY_DIAGONAL_DIRECTION
    };

    BOOST_CHECK_EQUAL(it1.direction(), AxisType::UpRight);
    BOOST_CHECK_EQUAL(it2.direction(), AxisType::UpLeft);
    BOOST_CHECK_EQUAL(it3.direction(), AxisType::DownRight);
    BOOST_CHECK_EQUAL(it4.direction(), AxisType::DownLeft);

    // 1st iterator can move 1 time before it reaches destination, 1 time before the end
    BOOST_CHECK(it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), -1);
    BOOST_CHECK_EQUAL(it1.moveForward(), EMPTY);

    BOOST_CHECK(!it1.hasNext());
    BOOST_CHECK_EQUAL(it1.distanceFromDestination(), 0);
}


BOOST_AUTO_TEST_SUITE_END()
