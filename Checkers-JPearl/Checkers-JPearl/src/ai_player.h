#pragma once

#include "checker_board.h"
#include "player.h"
namespace checkers
{
	class Game;
	class CheckerBoard;

	struct MoveHistory
	{
		CompactCoordinate from, to;
		CheckerPiece *piece;
	};
	
	class AiPlayer : public Player
	{
		static const int kNumHistoryRemembered = 32;

		Game *game_;
		int recurseLevels_;
		int currentHistoryIndex_;
		MoveHistory historyRemembered_[kNumHistoryRemembered];


		// Evaluates the value of a given board state. Negative in favor of O and positive in favor of X
		double evaluateBoardState(const CheckerBoard& board) const;
		// Evaluates the value of a given move and how the game could play from that point on. Negative in favor of O and positive in favor of X
		double evaluateMove(const Move& move, PieceSide side, double previousBoardScore, int recurseLevels, double &intrinsicScore) const;
		// Returns whether the given move has already been made recently
		bool isMoveInHistory(const Move& move) const;
		// Finds the best move in terms of possible value from all available moves. Returns the move most in favor of the given side and its score in the outBestScore parameter. Also keeps track of worst score
		Move* findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, int recurseLevels, double& outBestScore, double& outWorstScore, bool useHistory = false ) const;
	public:
		AiPlayer(Game * game, int recurseLevels);

		Move requestMove() override;
	};
}

