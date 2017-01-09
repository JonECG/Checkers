#pragma once

#include "player.h"
namespace checkers
{
	class Game;
	class CheckerBoard;
	class AiPlayer : public Player
	{
		Game *game_;
		int recurseLevels_;

		void jumpExplorationRecursion(const Move& moveToExplore, CheckerPiece * target, Move * moves, int * currentIndex) const;
		// Evaluates the value of a given board state. Negative in favor of O and positive in favor of X
		double evaluateBoardState(const CheckerBoard& board) const;
		// Evaluates the value of a given move and how the game could play from that point on. Negative in favor of O and positive in favor of X
		double evaluateMove(const Move& move, int recurseLevels) const;
	public:
		AiPlayer(Game * game, int recurseLevels);

		Move requestMove() const override;
	};
}

