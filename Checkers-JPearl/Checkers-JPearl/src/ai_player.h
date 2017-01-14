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
			static const int kNumWeights = 9;
			static const char * const kWeightNames[kNumWeights]; // Did someone say const?

			// Valuing pieces
			double pointsForMenAtHomeRow = 1;
			double pointsForMenAtKingRow = 1.1;
			double pointsForKing = 1.3;

			// Biases to promote cohesion
			double pointsForMoveAvailable = 0.01;
			double pointsForPieceInCenter = 0.02;

			// Where a piece wants to be located in respect to other pieces
			double pointsPerAdjacentAlly = 0.01;
			double pointsPerAdjacentBoundary = 0.01;
			double pointsPerAdjacentOpponent = -0.01;
			double pointsPerAdjacentEmptySpace = 0.01;

			// Gets all values in text format
			friend std::ostream& operator<< (std::ostream& stream, const Brain& brain);
		};
		static const Brain kDefaultBrain;

		union BrainView
		{
			double raw[Brain::kNumWeights];
			Brain brain;
		};

	private:
		int recurseLevels_;
		Brain brain_;

		// Evaluates the value of a given board state. Negative in favor of O and positive in favor of X
		double evaluateBoardState(const Game * game) const;
		// Evaluates the value of a given move and how the game could play from that point on. Negative in favor of O and positive in favor of X
		double evaluateMove(const Move& move, PieceSide side, double previousBoardScore, double &intrinsicScore, int currentRecursionLevel) const;
		// Finds the best move in terms of possible value from all available moves. Returns the move most in favor of the given side and its score in the outBestScore parameter. Also keeps track of worst score
		Move* findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, double& outBestScore, double& outWorstScore, int currentRecursionLevel = 0 ) const;
	public:
		

		AiPlayer(int recurseLevels, Brain brain = kDefaultBrain);

		const char * getDescriptor() const override;
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}

#endif // AI_PLAYER_H
