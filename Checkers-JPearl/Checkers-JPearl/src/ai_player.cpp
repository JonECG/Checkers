#include "ai_player.h"

#include <thread>
#include <chrono>
#include <iostream>

#include "move.h"
#include "game.h"

namespace checkers
{
	
	AiPlayer::AiPlayer(Game * game, int recurseLevels)
	{
		game_ = game;
		recurseLevels_ = recurseLevels;
	}

	double AiPlayer::evaluateBoardState(const CheckerBoard & board) const
	{
		// Valuing pieces
		const double kPointsForMenAtHomeRow = 1;
		const double kPointsForMenAtKingRow = 1.1;
		const double kPointsForKing = 1.3;

		// Small biases to promote cohesion
		const double kPointsForMoveAvailable = 0.01;
		const double kPointsForPieceInCenter = 0.02;

		double score = 0;

		// Evaluate Pieces
		for (int x = 0; x < CheckerBoard::kNumColumns; x++)
		{
			for (int y = 0; y < CheckerBoard::kNumRows; y++)
			{
				CompactCoordinate coord = CompactCoordinate();
				coord.column = x; coord.row = y;

				if (board.isCoordValid(coord))
				{
					CheckerPiece *piece = board.getPiece(coord);
					if (piece != nullptr)
					{
						PieceSide side = piece->getSide();
						int multiplier = (side == PieceSide::X) ? 1 : -1;

						if (piece->getIsKing())
						{
							score += kPointsForKing * multiplier;
						}
						else
						{
							double startRow = (side == PieceSide::O) ? 0 : CheckerBoard::kNumRows - 1;
							double endRow = (side == PieceSide::O) ? CheckerBoard::kNumRows - 1 : 0;
							double progress = (y - startRow) / (endRow - startRow);
							score += multiplier * std::abs(kPointsForMenAtHomeRow + (kPointsForMenAtKingRow - kPointsForMenAtHomeRow)*(progress));
						}

						// Manhatten distance
						double distToCenter = std::abs((CheckerBoard::kNumColumns / 2) - coord.column) + std::abs((CheckerBoard::kNumRows / 2) - coord.row);
						double maxDist = CheckerBoard::kNumColumns / 2 + CheckerBoard::kNumRows / 2;
						double distPercent = 1 - (distToCenter / maxDist);
						score += multiplier * distPercent * kPointsForPieceInCenter;
					}
				}
			}
		}

		// Evaluate Num Moves each
		int count = 0;
		game_->canAnyPieceMove(PieceSide::O, false, false, &count);
		score -= count * kPointsForMoveAvailable;
		game_->canAnyPieceMove(PieceSide::X, false, false, &count);
		score += count * kPointsForMoveAvailable;

		return score;
	}

	double AiPlayer::evaluateMove(const Move &move, PieceSide side, double previousBoardScore, int recurseLevels, double &intrinsicScore) const
	{
		CheckerBoard simulatedBoard = CheckerBoard();
		simulatedBoard.initialize(*game_->checkerBoard_);

		// Sneakily swap the checkerboard from the game with the simulated board -- switch it back before stack frame is popped
		CheckerBoard *originalBoard = game_->checkerBoard_;
		game_->checkerBoard_ = &simulatedBoard;

		game_->attemptMove(move);

		double boardScore = evaluateBoardState(simulatedBoard);
		double score = boardScore - previousBoardScore;
		intrinsicScore = score;

		bool winningMove = game_->checkForWinCondition(side);
		if (winningMove)
			score += (side == PieceSide::O) ? -100 : 100;

		if (recurseLevels > 0 && !winningMove)
		{
			// From this board state find all moves the opponent can make
			
			Move moves[Game::kMoveArraySize];
			double futureBestScore = 0;
			double futureWorstScore = 0;

			PieceSide otherSide = (side == PieceSide::O) ? PieceSide::X : PieceSide::O;

			findBestMove(moves, Game::kMoveArraySize, otherSide, boardScore, recurseLevels - 1, futureBestScore, futureWorstScore);

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

		game_->checkerBoard_ = originalBoard;
		simulatedBoard.release();

		return score;
	}

	bool AiPlayer::isMoveInHistory(const Move & move) const
	{
		CheckerPiece *piece = game_->checkerBoard_->getPiece(move.getCoordinate(0));
		for (int i = 0; i < kNumHistoryRemembered; i++)
		{
			if (historyRemembered_[i].from.column == move.getCoordinate(0).column && 
				historyRemembered_[i].to.column == move.getCoordinate(1).column && 
				historyRemembered_[i].from.row == move.getCoordinate(0).row &&
				historyRemembered_[i].to.row == move.getCoordinate(1).row &&
				historyRemembered_[i].piece == piece)
				return true;
		}
		return false;
	}

	Move * AiPlayer::findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, int recurseLevels, double &outBestScore, double & outWorstScore, bool useHistory) const
	{
		int startIndex = 0;
		int numPossibleMoves = game_->findAllMoves(side, moves, capacity, startIndex);

		double intrinsicScoreStore = 0;

		if (numPossibleMoves == 0)
		{
			return nullptr;
		}


		if (numPossibleMoves == 1)
		{
			// Early out if only one move available
			outBestScore = evaluateMove(moves[startIndex], side, currentBoardScore, recurseLevels, intrinsicScoreStore);
			outWorstScore = outBestScore;

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
			if (useHistory && isMoveInHistory(moves[i + startIndex]))
				continue; // Skip evaluating move if it's been made recently

			double value = evaluateMove(moves[i + startIndex], side, currentBoardScore, recurseLevels, intrinsicScoreStore);
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

	Move AiPlayer::requestMove()
	{
		Move moves[Game::kMoveArraySize];
		double score = 0;
		double worstScore = 0;
		double boardScore = evaluateBoardState(*game_->checkerBoard_);
 		Move move = *findBestMove(moves, Game::kMoveArraySize, getControllingSide(), boardScore, recurseLevels_, score, worstScore, true);

		// If this was an adjacent move, add it to the history
		if (move.getNumCoords() == 2)
		{
			CompactCoordinate from = move.getCoordinate(0);
			CompactCoordinate to = move.getCoordinate(1);
			if (std::abs(to.column - from.column) == 1)
			{
				MoveHistory history = MoveHistory();
				history.from = from; history.to = to;
				history.piece = game_->checkerBoard_->getPiece(from);
				historyRemembered_[currentHistoryIndex_] = history;
				currentHistoryIndex_ = (currentHistoryIndex_ + 1) % kNumHistoryRemembered;
			}
		}

#ifdef DEBUG
		std::cout << "AI sees board state as  " << boardScore << std::endl;
		std::cout << "AI thinks move score is " << score << std::endl;
#endif // DEBUG

		return move;
	}
}