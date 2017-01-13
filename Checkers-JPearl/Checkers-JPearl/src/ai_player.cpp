#include "ai_player.h"

#include <iostream>

#include "move.h"
#include "game.h"

namespace checkers
{
	const AiPlayer::Brain AiPlayer::kDefaultBrain = AiPlayer::Brain();

	AiPlayer::AiPlayer(int recurseLevels, Brain brain)
	{
		recurseLevels_ = recurseLevels;
		brain_ = brain;
	}

	double AiPlayer::evaluateBoardState(const Game * game) const
	{
		double score = 0;

		// Evaluate Pieces
		for (int x = 0; x < CheckerBoard::kNumColumns; x++)
		{
			for (int y = 0; y < CheckerBoard::kNumRows; y++)
			{
				CompactCoordinate coord = CompactCoordinate();
				coord.column = x; coord.row = y;

				if (game->checkerBoard_->isCoordValid(coord))
				{
					CheckerPiece *piece = game->checkerBoard_->getPiece(coord);
					if (piece != nullptr)
					{
						PieceSide side = piece->getSide();
						int multiplier = (side == PieceSide::X) ? 1 : -1;

						// Piece Value
						if (piece->getIsKing())
						{
							score += brain_.pointsForKing * multiplier;
						}
						else
						{
							double startRow = (side == PieceSide::O) ? 0 : CheckerBoard::kNumRows - 1;
							double endRow = (side == PieceSide::O) ? CheckerBoard::kNumRows - 1 : 0;
							double progress = (y - startRow) / (endRow - startRow);
							score += multiplier * (brain_.pointsForMenAtHomeRow + (brain_.pointsForMenAtKingRow - brain_.pointsForMenAtHomeRow)*(progress));
						}

						// Manhatten distance
						double distToCenter = std::abs((CheckerBoard::kNumColumns / 2) - coord.column) + std::abs((CheckerBoard::kNumRows / 2) - coord.row);
						double maxDist = CheckerBoard::kNumColumns / 2 + CheckerBoard::kNumRows / 2;
						double distPercent = 1 - (distToCenter / maxDist);
						score += multiplier * distPercent * brain_.pointsForPieceInCenter;


						// Getting nearby squares
						for (int direction = 0; direction < 4; direction++)
						{
							char deltaX = (direction & 0b01) ? 1 : -1;
							char deltaY = (direction & 0b10) ? 1 : -1;

							CompactCoordinate adjacent = coord;
							adjacent.column += deltaX; adjacent.row += deltaY;

							if (!game->checkerBoard_->isCoordValid(adjacent))
							{
								score += multiplier * brain_.pointsPerAdjacentBoundary; // This direction is stopped by the end of the board
							}
							else
							{
								CheckerPiece *adjacentPiece = game->checkerBoard_->getPiece(adjacent);
								if (adjacentPiece)
									score += multiplier *((adjacentPiece->getSide() == side) ? brain_.pointsPerAdjacentAlly : brain_.pointsPerAdjacentOpponent );
								else
									score += multiplier * brain_.pointsPerAdjacentEmptySpace;
							}
						}
					}
				}
			}
		}

		// Evaluate Num Moves each
		int count = 0;
		game->canAnyPieceMove(PieceSide::O, false, false, &count);
		score -= count * brain_.pointsForMoveAvailable;
		game->canAnyPieceMove(PieceSide::X, false, false, &count);
		score += count * brain_.pointsForMoveAvailable;

		return score;
	}

	double AiPlayer::evaluateMove(const Move &move, PieceSide side, double previousBoardScore, double &intrinsicScore, int currentRecursionLevel) const
	{
		Game * game = getGame();

		CheckerBoard simulatedBoard = CheckerBoard();
		simulatedBoard.initialize(*game->checkerBoard_);

		// Sneakily swap the checkerboard from the game with the simulated board -- switch it back before stack frame is popped
		CheckerBoard *originalBoard = game->checkerBoard_;
		game->checkerBoard_ = &simulatedBoard;

		game->attemptMove(move);

		double boardScore = evaluateBoardState(game);
		double score = boardScore - previousBoardScore;
		intrinsicScore = score;

		bool winningMove = game->checkForWinCondition(side);
		if (winningMove)
			score += (side == PieceSide::O) ? -100 : 100;

		if (currentRecursionLevel < recurseLevels_ && !winningMove)
		{
			// From this board state find all moves the opponent can make
			
			Move moves[Game::kMoveArraySize];
			double futureBestScore = 0;
			double futureWorstScore = 0;

			PieceSide otherSide = (side == PieceSide::O) ? PieceSide::X : PieceSide::O;

			findBestMove(moves, Game::kMoveArraySize, otherSide, boardScore, futureBestScore, futureWorstScore, currentRecursionLevel + 1);

			static double kUncertaintyPenalty = 0.95;
			double predictScore = futureBestScore * kUncertaintyPenalty;

#ifdef DEBUG
			for (int i = recurseLevels; i <= recurseLevels_; i++)
			{
				std::cout << "    ";
			}
			std::cout << "Current Decision futureBestScore: " << futureBestScore << "  futureWorstScore: " << futureWorstScore << std::endl;
#endif // DEBUG

			// If the predicted score is even worse than the worst possible future score, clamp it to the worst score -- in the odd case all moves are the same weight uncertainty doesn't matter
			if ((otherSide == PieceSide::O && predictScore > futureWorstScore) || (otherSide == PieceSide::X && predictScore < futureWorstScore) )
				predictScore = futureWorstScore;

			score += predictScore;
		}

		game->checkerBoard_ = originalBoard;
		simulatedBoard.release();

		return score;
	}

	Move * AiPlayer::findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, double &outBestScore, double & outWorstScore, int currentRecursionLevel) const
	{
		int startIndex = 0;
		int numPossibleMoves = getGame()->findAllMoves(side, moves, capacity, startIndex);

		double intrinsicScoreStore = 0;

		if (numPossibleMoves == 0)
		{
			return nullptr;
		}


		if (numPossibleMoves == 1)
		{
			// Early out if only one move available and is the first level of recursion
			if (recurseLevels_ != 0)
			{
				outBestScore = evaluateMove(moves[startIndex], side, currentBoardScore, intrinsicScoreStore, currentRecursionLevel);
				outWorstScore = outBestScore;
			}
#ifdef DEBUG
			for (int i = recurseLevels; i < recurseLevels_; i++)
			{
				std::cout << "    ";
			}
			std::cout << "AI values " << ((side==PieceSide::O) ? "O:" : "X:") << moves[startIndex] << " at intrinsic " << intrinsicScoreStore << " and accumulatively " << outBestScore << std::endl;
#endif // DEBUG

			return moves + startIndex;
		}
		
		int bestMoveIndex = startIndex;
		bool found = false;

		for (int i = 0; i < numPossibleMoves; i++)
		{
			double value = evaluateMove(moves[i + startIndex], side, currentBoardScore, intrinsicScoreStore, currentRecursionLevel);
			// Find best value in favor of this side
			if ( !found || ( (value > outBestScore) ^ (side == PieceSide::O) ) )
			{
				bestMoveIndex = i + startIndex;
				outBestScore = value;
				
			}
			if (!found || ((value < outWorstScore) ^ (side == PieceSide::O)))
			{
				outWorstScore = value;
			}

#ifdef DEBUG
			for (int whiteSpace = recurseLevels; whiteSpace < recurseLevels_; whiteSpace++)
			{
				std::cout << "    ";
			}
			std::cout << "AI values " << ((side == PieceSide::O) ? "O:" : "X:") << moves[i + startIndex] << " at intrinsic " << intrinsicScoreStore << " and accumulatively " << value << std::endl;
#endif // DEBUG

			found = true;
		}

		return moves + bestMoveIndex;
	}

	const char * AiPlayer::getDescriptor() const
	{
		return "AI ";
	}

	Move AiPlayer::requestMove()
	{
		Move moves[Game::kMoveArraySize];
		double score = 0;
		double worstScore = 0;
		double boardScore = evaluateBoardState(getGame());
 		Move move = *findBestMove(moves, Game::kMoveArraySize, getControllingSide(), boardScore, score, worstScore);

#ifdef DEBUG
		std::cout << "AI sees board state as  " << boardScore << std::endl;
		std::cout << "AI thinks move score is " << score << std::endl;
#endif // DEBUG

		return move;
	}
	void AiPlayer::sendMessage(const char * message) const
	{
		message;
		// Do nothing
	}

	std::ostream & operator<<(std::ostream & stream, const AiPlayer::Brain & brain)
	{
		return stream << "\n_____Brain values_____\n" <<
			"pointsForMenAtHomeRow : " << brain.pointsForMenAtHomeRow << "\n" <<
			"pointsForMenAtKingRow : " << brain.pointsForMenAtKingRow << "\n" <<
			"pointsForKing : " << brain.pointsForKing << "\n" <<
			"pointsForMoveAvailable : " << brain.pointsForMoveAvailable << "\n" <<
			"pointsForPieceInCenter : " << brain.pointsForPieceInCenter << "\n" <<
			"pointsPerAdjacentAlly : " << brain.pointsPerAdjacentAlly << "\n" <<
			"pointsPerAdjacentBoundary : " << brain.pointsPerAdjacentBoundary << "\n" <<
			"pointsPerAdjacentOpponent : " << brain.pointsPerAdjacentOpponent << "\n" <<
			"pointsPerAdjacentEmptySpace : " << brain.pointsPerAdjacentEmptySpace << "\n" <<
			std::endl;
	}
}
