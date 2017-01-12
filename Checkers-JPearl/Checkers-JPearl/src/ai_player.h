#pragma once
#ifndef AI_PLAYER_H
#define AI_PLAYER_H

#include "checker_board.h"
#include "player.h"
namespace checkers
{
	class Game;
	class CheckerBoard;

	class AiPlayer : public Player
	{
	public:

		struct Brain
		{
			static const int kNumWeights = 5;
			// Valuing pieces
			double pointsForMenAtHomeRow = 1;
			double pointsForMenAtKingRow = 1.1;
			double pointsForKing = 1.3;

			// Small biases to promote cohesion
			double pointsForMoveAvailable = 0.01;
			double pointsForPieceInCenter = 0.02;
		} static const kDefaultBrain;
		
		union BrainView
		{
			double raw[Brain::kNumWeights];
			Brain brain;
		};

	private:
		static const int kNumHistoryRemembered = 32;
		struct MoveHistory
		{
			CompactCoordinate from, to;
			CheckerPiece *piece;
		} historyRemembered_[kNumHistoryRemembered];
		int currentHistoryIndex_;

		int recurseLevels_;
		Brain brain_;

		// Evaluates the value of a given board state. Negative in favor of O and positive in favor of X
		double evaluateBoardState(const Game * game) const;
		// Evaluates the value of a given move and how the game could play from that point on. Negative in favor of O and positive in favor of X
		double evaluateMove(const Move& move, PieceSide side, double previousBoardScore, int recurseLevels, double &intrinsicScore) const;
		// Returns whether the given move has already been made recently
		bool isMoveInHistory(const Move& move) const;
		// Finds the best move in terms of possible value from all available moves. Returns the move most in favor of the given side and its score in the outBestScore parameter. Also keeps track of worst score
		Move* findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, int recurseLevels, double& outBestScore, double& outWorstScore, bool useHistory = false ) const;
	public:
		

		AiPlayer(int recurseLevels, Brain brain = kDefaultBrain);

		const char * getDescriptor() const override;
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}

#endif // AI_PLAYER_H
