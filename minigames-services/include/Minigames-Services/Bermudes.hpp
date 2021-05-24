#ifndef RPT_MINIGAMES_SERVICES_BERMUDES_HPP
#define RPT_MINIGAMES_SERVICES_BERMUDES_HPP

/**
 * @file Bermudes.hpp
 */

#include <Minigames-Services/AxisIterator.hpp>
#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


/// Implements RpT-Minigame "Bermudes"
class Bermudes : public BoardGame {
private:
    /// One of the 2 available moves for this game
    enum struct Move {
        Elimination, Flip
    };

    static constexpr std::initializer_list<std::initializer_list<Square>> INITIAL_GRID_ {
            { BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK },
            { BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK },
            { BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK },
            { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
            { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
            { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
            { WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE },
            { WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE },
            { WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE },
    };

    /// Checks for every squares between origin and destination ,or distance-from-destination square if `until` is
    /// specified, to be empty
    static void checkFreeTrajectory(AxisIterator& move_trajectory, int until = 0);

    std::optional<Move> last_move_;

    /// Tries to perform given move as elimination-take, saving grid modifications into given reference argument
    void playElimination(GridUpdate& updates, AxisIterator move);
    /// Tries to perform given move as flip-take, saving grid modifications into given reference argument
    void playFlip(GridUpdate& updates, AxisIterator move);

public:
    /// Constructs Bermudes minigame with game-specific initial configuration (27 pawns for each color/player)
    Bermudes();

    /// Resets flips-take chain and switch to next player
    Player nextRound() override;

    /// Round is necessarily terminated if player did a elimination-take move
    bool isRoundTerminated() const override;

    /// Two available moves: elimination-take or flips-take, then flips-take chaining
    GridUpdate play(const Coordinates &from, const Coordinates &to) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_BERMUDES_HPP
