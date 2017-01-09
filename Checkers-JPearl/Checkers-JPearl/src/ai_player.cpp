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

	void AiPlayer::jumpExplorationRecursion(const Move & moveToExplore, CheckerPiece * target, Move * moves, int& outCurrentIndex) const
	{
		CompactCoordinate endCoord = moveToExplore.getCoordinate(moveToExplore.getNumCoords() - 1);
		CompactCoordinate middleCoord = moveToExplore.getCoordinate(moveToExplore.getNumCoords() - 2);
		middleCoord.column = (middleCoord.column + endCoord.column) / 2;
		middleCoord.row = (middleCoord.row + endCoord.row) / 2;

		CheckerPiece *jumpedPiece = game_->checkerBoard_->getPiece(middleCoord);
		unsigned char prevMark = jumpedPiece->getMark();
		jumpedPiece->setMark(1);

		bool wasKing = target->getIsKing();
		if ((target->getSide() == PieceSide::X && endCoord.row == 0) || (target->getSide() == PieceSide::O && endCoord.row == CheckerBoard::kNumRows - 1))
			target->setIsKing(true);

		const int maxMovesPerPiece = 4;
		CompactCoordinate results[maxMovesPerPiece];
		int numResults = maxMovesPerPiece;

		game_->canMovePieceAt(endCoord, target, true, true, results, &numResults);

		if (numResults == 0) // End of this jump sequence
		{
			moves[outCurrentIndex++] = moveToExplore;
		}
		else
		{
			for (int i = 0; i < numResults; i++)
			{
				Move newMove = moveToExplore;
				newMove.addCoordinate(results[i]);
				jumpExplorationRecursion(newMove, target, moves, outCurrentIndex);
			}
		}

		
		target->setIsKing(wasKing);
		jumpedPiece->setMark(prevMark);
	}

	int AiPlayer::findAllMoves(PieceSide side, Move * moves, int moveCapacity, int& outStartPosition) const
	{
		CheckerBoard *cb = game_->checkerBoard_;

		// Store adjacent moves in the back array and jump moves in the front so they're divided if we're only concerned with jumps
		
		int currentMoveIndex = 0;
		int currentJumpIndex = 0;

		bool canJump = false;

		// Get all moves
		for (int x = 0; x < CheckerBoard::kNumColumns; x++)
		{
			for (int y = 0; y < CheckerBoard::kNumRows; y++)
			{
				CompactCoordinate coord = CompactCoordinate();
				coord.column = x; coord.row = y;

				if (cb->isCoordValid(coord))
				{
					CheckerPiece *piece = cb->getPiece(coord);
					if (piece != nullptr && piece->getSide() == side)
					{
						CompactCoordinate results[kMaxMovesPerPiece];
						int numResults = kMaxMovesPerPiece;
						game_->canMovePieceAt(coord, piece, false, false, results, &numResults);

						for (int i = 0; i < numResults; i++)
						{
							Move move = Move();
							move.addCoordinate(coord);
							move.addCoordinate(results[i]);

							int distance = std::abs(results[i].column - coord.column);
							if (distance == 1 && !canJump) // is a move
							{
								moves[moveCapacity - 1 - currentMoveIndex++] = move;
							}

							if (distance == 2) // is a jump
							{
								// Perform depth first exploration of jumps backed by the unused portion of the move array
								jumpExplorationRecursion(move, piece, moves, currentJumpIndex);
								canJump = true;
							}
						}
					}
				}
			}
		}

		int startIndex = 0, endIndex = 0;

		if (canJump)
		{
			startIndex = 0;
			endIndex = currentJumpIndex;
		}
		else
		{
			startIndex = moveCapacity - currentMoveIndex;
			endIndex = moveCapacity;
		}

		outStartPosition = startIndex;
		return endIndex - startIndex;
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

	double AiPlayer::evaluateMove(const Move &move, PieceSide side, double previousBoardScore, int recurseLevels) const
	{
		CheckerBoard simulatedBoard = CheckerBoard();
		simulatedBoard.initialize(*game_->checkerBoard_);

		// Sneakily swap the checkerboard from the game with the simulated board -- switch it back before stack frame is popped
		CheckerBoard *originalBoard = game_->checkerBoard_;
		game_->checkerBoard_ = &simulatedBoard;

		const char * error = game_->attemptMove(move);

		double boardScore = evaluateBoardState(simulatedBoard);
		double score = boardScore - previousBoardScore;

		bool winningMove = game_->checkForWinCondition(side);
		if (winningMove)
			score += (side == PieceSide::O) ? -100 : 100;

		if (recurseLevels > 0 && !winningMove)
		{
			// From this board state find all moves the opponent can make
			
			Move moves[kMoveArraySize];
			double futureBestScore = 0;
			double futureWorstScore = 0;

			PieceSide otherSide = (side == PieceSide::O) ? PieceSide::X : PieceSide::O;

			findBestMove(moves, kMoveArraySize, otherSide, boardScore, recurseLevels - 1, futureBestScore, futureWorstScore);

			static double kUncertaintyPenalty = 0.75;
			double predictScore = futureBestScore * kUncertaintyPenalty;

			// If the predicted score is even worse than the worst possible future score, clamp it to the worst score
			if ((predictScore < futureWorstScore && predictScore < futureBestScore) || (predictScore > futureWorstScore && predictScore > futureBestScore) )
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
		int numPossibleMoves = findAllMoves(side, moves, capacity, startIndex);

		if (numPossibleMoves == 0)
		{
			return nullptr;
		}


		if (numPossibleMoves == 1)
		{
			// Early out if only one move available
			outBestScore = evaluateMove(moves[startIndex], side, currentBoardScore, 0);
			outWorstScore = outBestScore;
			return moves + startIndex;
		}
		
		int bestMoveIndex = startIndex;
		bool found = false;

		for (int i = 0; i < numPossibleMoves; i++)
		{
			if (useHistory && isMoveInHistory(moves[i + startIndex]))
				continue; // Skip evaluating move if it's been made recently

			double value = evaluateMove(moves[i + startIndex], side, currentBoardScore, recurseLevels);
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

			if(useHistory)
				std::cout << "AI values  " << moves[i + startIndex] << " at " << value << std::endl;

			found = true;
		}

		return moves + bestMoveIndex;
	}

	Move AiPlayer::requestMove()
	{
		Move moves[kMoveArraySize];
		double score = 0;
		double worstScore = 0;
		double boardScore = evaluateBoardState(*game_->checkerBoard_);
 		Move move = *findBestMove(moves, kMoveArraySize, getControllingSide(), boardScore, recurseLevels_, score, worstScore, true);

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

		std::cout << "AI sees board state as  " << boardScore << std::endl;
		std::cout << "AI thinks move score is " << score << std::endl;

		// Adds a delay so the terminal is readable during AI turn
		if( recurseLevels_ < 4 )
			std::this_thread::sleep_for(std::chrono::milliseconds(300));

		return move;
	}
}