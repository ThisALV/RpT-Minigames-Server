#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/Grid.hpp>


using namespace MinigamesServices;


BOOST_AUTO_TEST_SUITE(GridTests)


// Constant shortcut for empty square inside grid
constexpr Square EMPTY { Square::Free };


/// Provides an empty grid of dimensions 5x10
class EmptyGridFixture {
public:
    static constexpr std::initializer_list<std::initializer_list<Square>> INITIAL_CONFIGURATION {
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    };

    Grid grid;

    EmptyGridFixture() : grid { INITIAL_CONFIGURATION } {}
};


/*
 * flip() free function unit tests
 */
BOOST_AUTO_TEST_SUITE(Flip)


BOOST_AUTO_TEST_CASE(Free) {
    BOOST_CHECK_THROW(flip(Square::Free), BadSquareState);
}

BOOST_AUTO_TEST_CASE(White) {
    BOOST_CHECK_EQUAL(flip(Square::White), Square::Black);
}

BOOST_AUTO_TEST_CASE(Black) {
    BOOST_CHECK_EQUAL(flip(Square::Black), Square::White);
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * Ctor unit tests
 */
BOOST_AUTO_TEST_SUITE(Constructor)


BOOST_AUTO_TEST_CASE(ZeroLines) {
    BOOST_CHECK_THROW((Grid { {} }), BadDimensions);
}

BOOST_AUTO_TEST_CASE(ManyLinesZeroColumns) {
    BOOST_CHECK_THROW((Grid { {}, {}, {} }), BadDimensions);
}

BOOST_AUTO_TEST_CASE(TooManyLines) {
    // Dimensions: 30 lines of 1 column
    const std::initializer_list<std::initializer_list<Square>> initial_configuration {
            { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
            { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
            { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
    };

    BOOST_CHECK_THROW((Grid { initial_configuration }), BadDimensions);

}

BOOST_AUTO_TEST_CASE(TooManyColumns) {
    // Dimensions: 1 line of 30 columns
    const std::initializer_list<std::initializer_list<Square>> initial_configuration {
            {
                EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
                EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
                EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
            }
    };

    BOOST_CHECK_THROW((Grid { initial_configuration }), BadDimensions);
}

BOOST_AUTO_TEST_CASE(ManyLinesDifferentColumnsCount) {
    // 1st line has dimension 2, 2nd line has dim 1 and 3rd one has dim 3, no valid 2D dimensions available for the
    // matrix
    const std::initializer_list<std::initializer_list<Square>> initial_configuration {
            { EMPTY, EMPTY },
            { EMPTY },
            { EMPTY, EMPTY, EMPTY }
    };

    BOOST_CHECK_THROW((Grid { initial_configuration }), BadDimensions);
}

BOOST_AUTO_TEST_CASE(ManyLinesSameColumnsCount) {
    // Every line has dimension 3, 2D dimension available for the matrix: 3x3
    const std::initializer_list<std::initializer_list<Square>> initial_configuration {
            { EMPTY, EMPTY, EMPTY },
            { EMPTY, EMPTY, EMPTY },
            { EMPTY, EMPTY, EMPTY }
    };

    BOOST_CHECK_NO_THROW((Grid { initial_configuration }));
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * isInsideGrid() method unit tests
 */
BOOST_FIXTURE_TEST_SUITE(IsInsideGrid, EmptyGridFixture)


/*
 * Should return true
 */
BOOST_AUTO_TEST_SUITE(InsideGrid)


BOOST_AUTO_TEST_CASE(InsideGridMiddle) {
    BOOST_CHECK(grid.isInsideGrid({ 2, 3 }));
}

BOOST_AUTO_TEST_CASE(InsideGridUpLeftCorner) {
    BOOST_CHECK(grid.isInsideGrid({ 1, 1 }));
}

BOOST_AUTO_TEST_CASE(InsideGridDownRightCorner) {
    BOOST_CHECK(grid.isInsideGrid({ 10, 5 }));
}


BOOST_AUTO_TEST_SUITE_END()



/*
 * Should return false
 */
BOOST_AUTO_TEST_SUITE(OutsideGrid)


BOOST_AUTO_TEST_CASE(TooLargeX) {
    BOOST_CHECK(!grid.isInsideGrid({ 10, 6 }));
}

BOOST_AUTO_TEST_CASE(TooSmallX) {
    BOOST_CHECK(!grid.isInsideGrid({ 10, 0 }));
}

BOOST_AUTO_TEST_CASE(TooLargeY) {
    BOOST_CHECK(!grid.isInsideGrid({ 11, 5 }));
}

BOOST_AUTO_TEST_CASE(TooSmallY) {
    BOOST_CHECK(!grid.isInsideGrid({ 0, 5 }));
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()


/*
 * Subscript operator unit tests
 */
BOOST_FIXTURE_TEST_SUITE(SubscriptOperator, EmptyGridFixture)


BOOST_AUTO_TEST_CASE(OutsideGrid) {
    BOOST_CHECK_THROW((grid[{ -1, 6 }]), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(InsideGrid) {
    grid[{ 2, 3 }] = Square::White;
    grid[{ 1, 1 }] = Square::Black;

    BOOST_CHECK_EQUAL((grid[{ 3, 2 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 2, 3 }]), Square::White);
    BOOST_CHECK_EQUAL((grid[{ 1, 1 }]), Square::Black);
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
