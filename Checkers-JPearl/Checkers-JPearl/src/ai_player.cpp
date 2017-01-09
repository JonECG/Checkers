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

		CheckerPiece *jumpedPiece = game_->checkerBoard_.getPiece(middleCoord);
		unsigned char prevMark = jumpedPiece->getMark();
		jumpedPiece->setMark(1);

		bool wasKing = target->getIsKing();
		if ((target->getSide() == PieceSide::X && endCoord.row == 0) || (target->getSide() == PieceSide::O && endCoord.row == CheckerBoard::kNumRows - 1))
			target->setIsKing(true);

		const int maxJumpsPerPiece = 4;
		CompactCoordinate results[maxJumpsPerPiece];
		int numResults = maxJumpsPerPiece;

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

	Move AiPlayer::requestMove() const
	{
		CheckerBoard &cb = game_->checkerBoard_;

		// Store adjacent moves in the back array and jump moves in the front so they're divided if we're only concerned with jumps
		const int maxMovesPerPiece = 8;
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

				if (cb.isCoordValid(coord))
				{
					CheckerPiece *piece = game_->checkerBoard_.getPiece(coord);
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

		if (canJump)
		{
			return moves[std::rand() % currentJumpIndex];
		}
		else
		{
			int index = std::rand() % (currentJumpIndex + currentMoveIndex);
			index = (index + moveArraySize - currentMoveIndex) % moveArraySize;
			return moves[index];
		}

	}
}