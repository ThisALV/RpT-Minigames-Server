#ifndef RPT_MINIGAMES_SERVICES_CANARIES_HPP
#define RPT_MINIGAMES_SERVICES_CANARIES_HPP

/**
 * @file Canaries.hpp
 */

#include <Minigames-Services/AxisIterator.hpp>
#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


/// Implements RpT-Minigame "Canaries"
class Canaries : public BoardGame {
private:
    static constexpr std::initializer_list<std::initializer_list<Square>> INITIAL_GRID_ {
            { BLACK, BLACK, BLACK, BLACK },
            { BLACK, BLACK, BLACK, BLACK },
            { WHITE, WHITE, WHITE, WHITE },
            { WHITE, WHITE, WHITE, WHITE }
    };

    /// Tries to perform given move as normal, saving grid modifications into given reference argument
    void playNormal(GridUpdate& updates, AxisIterator move);
    /// Tries to perform given move as eat, saving grid modifications into given reference argument
    void playEat(GridUpdate& updates, AxisIterator move);

    /// Checks if any move, normal or eat, can be performed by given player
    bool isBlocked(Player player) const;

public:
    /// Constructs Canaries minigame with game-specific initial configuration (8 pawns for each color/player)
    Canaries();

    /// A player won if his opponent no longer have any pawn on the grid
    std::optional<Player> victoryFor() const override;

    /// Round is terminated if player as move, no chaining available in this game
    bool isRoundTerminated() const override;

    /// Two available moves: normal or jump, then jumps chaining
    GridUpdate play(const Coordinates &from, const Coordinates &to) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_CANARIES_HPP
