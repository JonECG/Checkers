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
		const double kPointsForMenAtHomeRow = 5;
		const double kPointsForMenAtKingRow = 6;
		const double kPointsForKing = 7;

		// Small biases to promote cohesion
		const double kPointsForMoveAvailable = 0.2;
		const double kPenaltyForAdjacentOpenSquare = 0.1;

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
					}
				}
			}
		}

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
			double futureScore = 0;

			PieceSide otherSide = (side == PieceSide::O) ? PieceSide::X : PieceSide::O;

			findBestMove(moves, kMoveArraySize, otherSide, boardScore, recurseLevels - 1, futureScore);

			static double kUncertaintyPenalty = 0.99;
			score += futureScore * kUncertaintyPenalty;
		}

		game_->checkerBoard_ = originalBoard;
		simulatedBoard.release();

		return score;
	}

	Move * AiPlayer::findBestMove(Move *moves, int capacity, PieceSide side, double currentBoardScore, int recurseLevels, double & outBestScore, bool useHistory) const
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
			return moves;
		}
		
		int bestMoveIndex = startIndex;
		double bestValue = 0;
		bool found = false;

		for (int i = 0; i < numPossibleMoves; i++)
		{
			double value = evaluateMove(moves[i + startIndex], side, currentBoardScore, recurseLevels);
			// Find best value in favor of this side
			if ( !found || (value > bestValue ^ side == PieceSide::O) )
			{
				bestMoveIndex = i + startIndex;
				bestValue = value;
				found = true;
			}
		}

		outBestScore = bestValue;
		return moves + bestMoveIndex;
	}

	Move AiPlayer::requestMove()
	{
		Move moves[kMoveArraySize];
		double score = 0;
		double boardScore = evaluateBoardState(*game_->checkerBoard_);
 		Move move = *findBestMove(moves, kMoveArraySize, getControllingSide(), boardScore, recurseLevels_, score, true);


		std::cout << "AI sees board state as  " << boardScore << std::endl;
		std::cout << "AI thinks move score is " << score << std::endl;

		// Adds a delay so the terminal is readable during AI turn
		if( recurseLevels_ < 4 )
			std::this_thread::sleep_for(std::chrono::milliseconds(300));

		return move;
	}
}