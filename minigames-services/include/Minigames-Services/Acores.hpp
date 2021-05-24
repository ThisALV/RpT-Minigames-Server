#ifndef RPT_MINIGAMES_SERVICES_ACORES_HPP
#define RPT_MINIGAMES_SERVICES_ACORES_HPP

/**
 * @file Acores.hpp
 */

#include <Minigames-Services/AxisIterator.hpp>
#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


/// Implements RpT-Minigame "Açores"
class Acores : public BoardGame {
private:
    /// One of the 2 available moves for this game
    enum struct Move {
        Normal, Jump
    };

    static constexpr std::initializer_list<std::initializer_list<Square>> INITIAL_GRID_ {
            { WHITE, WHITE, WHITE, BLACK, BLACK },
            { WHITE, WHITE, WHITE, BLACK, BLACK },
            { WHITE, WHITE, EMPTY, BLACK, BLACK },
            { WHITE, WHITE, BLACK, BLACK, BLACK },
            { WHITE, WHITE, BLACK, BLACK, BLACK },
    };

    std::optional<Move> last_move_;

    /// Tries to perform given move as normal, saving grid modifications into given reference argument
    void playNormal(GridUpdate& updates, AxisIterator move);
    /// Tries to perform given move as jump with eat, saving grid modifications into given reference argument
    void playJump(GridUpdate& updates, AxisIterator move);

public:
    /// Constructs Açores minigame with game-specific initial configuration (12 pawns for each color/player)
    Acores();

    /// Resets jumps chain and switch to next player
    Player nextRound() override;

    /// Round is terminated if either last move if normal or player manually ended round
    bool isRoundTerminated() const override;

    /// Two available moves: normal or jump, then jumps chaining
    GridUpdate play(const Coordinates &from, const Coordinates &to) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_ACORES_HPP
