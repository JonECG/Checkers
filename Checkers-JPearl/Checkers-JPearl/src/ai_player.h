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

		static const int kMaxMovesPerPiece = 4;
		static const int kMoveArraySize = CheckerBoard::kNumPiecesPerPlayer*kMaxMovesPerPiece;

		Game *game_;
		int recurseLevels_;
		int currentHistoryIndex_;
		MoveHistory historyRemembered_[kNumHistoryRemembered];

		// Used to traverse all jump paths, storing them in the moves array and updating outCurrentIndex to the current first availabe spot
		void jumpExplorationRecursion(const Move& moveToExplore, CheckerPiece * target, Move * moves, int& outCurrentIndex) const;
		// Find all valid moves for the given side, returns the number of moves available and stored starting from the index returned in outStartPosition
		int findAllMoves(PieceSide side, Move * moves, int moveCapacity, int& outStartPosition) const;

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

