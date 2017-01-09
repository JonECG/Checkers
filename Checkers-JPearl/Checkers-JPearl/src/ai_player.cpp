#include "ai_player.h"

#include <thread>
#include <chrono>

#include "move.h"
#include "game.h"

namespace checkers
{
	
	AiPlayer::AiPlayer(Game * game, int recurseLevels)
	{
		game_ = game;
		recurseLevels_ = recurseLevels;
	}

	void AiPlayer::jumpExplorationRecursion(const Move & moveToExplore, CheckerPiece * target, Move * moves, int * currentIndex) const
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
			moves[(*currentIndex)++] = moveToExplore;
		}
		else
		{
			for (int i = 0; i < numResults; i++)
			{
				Move newMove = moveToExplore;
				newMove.addCoordinate(results[i]);
				jumpExplorationRecursion(newMove, target, moves, currentIndex);
			}
		}

		
		target->setIsKing(wasKing);
		jumpedPiece->setMark(prevMark);
	}

	double AiPlayer::evaluateBoardState(const CheckerBoard & board) const
	{
		// Valuing pieces
		const double kPointsForMenAtHomeRow = 2;
		const double kPointsForMenAtKingRow = 3;
		const double kPointsForKing = 4;

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
							score += multiplier * (kPointsForMenAtHomeRow + (kPointsForMenAtKingRow - kPointsForMenAtHomeRow)*(progress));
						}
					}
				}
			}
		}

		return score;
	}

	double AiPlayer::evaluateMove(const Move & move, int recurseLevels) const
	{
		CheckerBoard simulatedBoard = CheckerBoard();
		simulatedBoard.initialize(*game_->checkerBoard_);

		// Sneakily swap the checkerboard from the game with the simulated board -- switch it back before stack frame is popped
		CheckerBoard *original = game_->checkerBoard_;
		game_->checkerBoard_ = &simulatedBoard;

		const char * error = game_->attemptMove(move);

		double score = evaluateBoardState(simulatedBoard);

		// Recurse for potential future states
		recurseLevels;

		game_->checkerBoard_ = original;
		simulatedBoard.release();

		return score;
	}

	Move AiPlayer::requestMove() const
	{
		CheckerBoard *cb = game_->checkerBoard_;

		// Store adjacent moves in the back array and jump moves in the front so they're divided if we're only concerned with jumps
		const int maxMovesPerPiece = 4;
		const int moveArraySize = CheckerBoard::kNumPiecesPerPlayer*maxMovesPerPiece;
		Move moves[moveArraySize];
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
					if (piece != nullptr && piece->getSide() == getControllingSide())
					{
						CompactCoordinate results[maxMovesPerPiece];
						int numResults = maxMovesPerPiece;
						game_->canMovePieceAt(coord, piece, false, false, results, &numResults);

						for (int i = 0; i < numResults; i++)
						{
							Move move = Move();
							move.addCoordinate(coord);
							move.addCoordinate(results[i]);

							int distance = std::abs(results[i].column - coord.column);
							if (distance == 1 && !canJump) // is a move
							{
								moves[moveArraySize - 1 - currentMoveIndex++] = move;
							}

							if (distance == 2) // is a jump
							{
								// Perform depth first exploration of jumps backed by the unused portion of the move array
								jumpExplorationRecursion(move, piece, moves, &currentJumpIndex);
								canJump = true;
							}
						}
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		int startIndex = 0, endIndex = 0;

		if (canJump)
		{
			startIndex = 0; 
			endIndex = currentJumpIndex;
		}
		else
		{
			startIndex = moveArraySize - currentMoveIndex;
			endIndex = moveArraySize;
		}

		Move bestMovement = moves[startIndex];
		double bestValue = evaluateMove(bestMovement, recurseLevels_);
		for (int i = startIndex + 1; i < endIndex; i++)
		{
			double value = evaluateMove(moves[i], recurseLevels_);
			// Find best value in favor of this side
			if (value > bestValue ^ getControllingSide() == PieceSide::O)
			{
				bestMovement = moves[i];
				bestValue = value;
			}
		}

		return bestMovement;
	}
}