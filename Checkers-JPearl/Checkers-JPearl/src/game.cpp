#include "game.h"

#include <iostream>
#include <vector>

#include "player.h"
#include "ai_player.h"
#include "local_player.h"
#include "move.h"

namespace checkers
{
	Game::Game()
	{
		currentPlayerTurn_ = 0;
	}

	void Game::initialize()
	{
		checkerBoard_ = new CheckerBoard();
		checkerBoard_->initialize();
		players_ = new Player*[2];
		numPlayers_ = 2;

		players_[PieceSide::O] = new AiPlayer(this, 5);
		players_[PieceSide::X] = new AiPlayer(this, 3);

		players_[PieceSide::O]->setControllingSide(PieceSide::O);
		players_[PieceSide::X]->setControllingSide(PieceSide::X);
	}

	void Game::release()
	{
		checkerBoard_->release();
		delete checkerBoard_;

		for (int i = 0; i < numPlayers_; i++)
		{
			delete players_[i];
		}
		delete[] players_;
	}

	void Game::runMoves(char ** moves, int numMoves)
	{
		for (int i = 0; i < numMoves; i++)
		{
			try
			{
				Move move = Move::parseFromString(moves[i]);
				attemptMove(move);
			}
			catch (void* ignore) { ignore; }

		}
	}

	bool Game::canMovePieceAt(CompactCoordinate coord, CheckerPiece *piece, bool onlyJumpMoves, bool ignoreMarked, CompactCoordinate * coordinates, int * numCoordinates ) const
	{
		int maxIndex = 0;
		
		if (numCoordinates != nullptr)
		{
			maxIndex = *numCoordinates;
			*numCoordinates = 0;
		}

		if (piece == nullptr)
			piece = checkerBoard_->getPiece(coord);

		if (piece == nullptr)
			return false; // No piece at given position

		for (int direction = 0; direction < 4; direction++)
		{
			int deltaX = (direction & 0b01) ? 1 : -1;
			int deltaY = (direction & 0b10) ? 1 : -1;

			if (!piece->getIsKing() && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
				continue; // The piece cannot move this way

			CompactCoordinate move = coord;
			move.column += deltaX; move.row += deltaY;

			if (!checkerBoard_->isCoordValid(move))
				continue; // This direction is stopped by the end of the board


			CheckerPiece *pieceAtMove = checkerBoard_->getPiece(move);

			if (!onlyJumpMoves && (pieceAtMove == nullptr || pieceAtMove == piece || (ignoreMarked && pieceAtMove->getMark()) ) ) // Treats space as empty if it's the space the piece actually occupies during simulation
			{
				if (coordinates != nullptr && *numCoordinates < maxIndex )
				{
					coordinates[(*numCoordinates)++] = move;
				}
				else
				{
					return true;
				}
			}
			else
			{
				CompactCoordinate jump = move;
				jump.column += deltaX; jump.row += deltaY;

				if (!checkerBoard_->isCoordValid(jump))
					continue; // This direction is stopped by the end of the board

				CheckerPiece *pieceAtJump = checkerBoard_->getPiece(jump);

				if (pieceAtMove != nullptr && pieceAtMove->getSide() != piece->getSide() && !(ignoreMarked && pieceAtMove->getMark()) && (pieceAtJump == nullptr || pieceAtJump == piece || (ignoreMarked && pieceAtJump->getMark())))
				{
					if (coordinates != nullptr && *numCoordinates < maxIndex)
					{
						coordinates[(*numCoordinates)++] = jump;
					}
					else
					{
						return true;
					}
				}
			}
		}

		return (coordinates != nullptr) && (*numCoordinates > 0);
	}

	bool Game::canAnyPieceMove(PieceSide side, bool onlyJumpMoves, bool ignoreMarked) const
	{
		for (int x = 0; x < CheckerBoard::kNumColumns; x++)
		{
			for (int y = 0; y < CheckerBoard::kNumRows; y++)
			{
				CompactCoordinate coord = CompactCoordinate();
				coord.column = x; coord.row = y;

				if (checkerBoard_->isCoordValid(coord))
				{
					CheckerPiece *piece = checkerBoard_->getPiece(coord);
					if (piece != nullptr && piece->getSide() == side)
					{
						if (canMovePieceAt(coord, piece, onlyJumpMoves, ignoreMarked))
							return true; // Early out
					}
				}
			}
		}
		return false;
	}

	// All of the hard work of move validation happens here
	const char * Game::attemptMoveInternal(const Move & move, void * data)
	{
		CompactCoordinate startCoord = move.getCoordinate(0);

		if (!checkerBoard_->isCoordValid(startCoord))
			return "Start position was not valid";

		CheckerPiece *piece = checkerBoard_->getPiece(startCoord);

		if (!piece)
			return "No checker in start position";

		// Board and piece state do not change until the move is completed
		bool treatAsKing = piece->getIsKing();
		std::vector<CompactCoordinate> *toBeRemoved = reinterpret_cast<std::vector<CompactCoordinate>*>(data);
		bool hadJumpedDuringMove = false;

		CompactCoordinate previousCoord = startCoord;
		for (int i = 1; i < move.getNumCoords(); i++)
		{
			CompactCoordinate currentCoord = move.getCoordinate(i);

			if (!checkerBoard_->isCoordValid(currentCoord))
				return "An intermediate position was not valid";

			int deltaX = currentCoord.column - previousCoord.column;
			int deltaY = currentCoord.row - previousCoord.row;

			if (std::abs(deltaX) != std::abs(deltaY))
				return "Checkers can only move diagonally";

			int distance = std::abs(deltaY);

			switch (distance)
			{
			case 0:
				return "Checkers have to move at least one space";
			case 1:

				if (move.getNumCoords() > 2)
					return "Moves to adjacent squares can only be made in a singular move";

				if (!treatAsKing && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
					return "The piece tried to move in a direction it was not allowed to";

				if (checkerBoard_->getPiece(currentCoord) != nullptr)
					return "The piece tried to move into a space that is already occupied";

				if (canAnyPieceMove(piece->getSide(), true))
					return "Can only make an adjacent move if no jumps are available";

				break;
			case 2:
			{
				// TODO: Men capturing backwards could be implemented here by removing this check
				if (!treatAsKing && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
					return "The piece tried to jump in a direction it was not allowed to";

				if (checkerBoard_->getPiece(currentCoord) != nullptr && checkerBoard_->getPiece(currentCoord) != piece) // Treats space as empty if it's the one that's moving
					return "The piece tried to jump into a space that is already occupied";

				CompactCoordinate middleCoord = previousCoord;
				middleCoord.column += deltaX / 2;
				middleCoord.row += deltaY / 2;
				CheckerPiece *middlePiece = checkerBoard_->getPiece(middleCoord);

				if (middlePiece == nullptr || middlePiece->getSide() == piece->getSide())
					return "The piece tried to jump over empty spaces or pieces of its color";

				// Mark piece so that it is ignored in future canMovePieceAt calls and register it to be removed if move is successful
				middlePiece->setMark(1);
				toBeRemoved->push_back(middleCoord);
				hadJumpedDuringMove = true;

				break;
			}
			default:
				// TODO: Flying kings could be implemented here

				return "The piece can't move that far";
			}


			// Evaluate if piece should be king from this step
			if ((piece->getSide() == PieceSide::X && currentCoord.row == 0) || (piece->getSide() == PieceSide::O && currentCoord.row == CheckerBoard::kNumRows - 1))
				treatAsKing = true;

			previousCoord = currentCoord;
		}

		// Treat the piece as king for the keepJumping check but undo it afterwards incase it invalidates this move
		bool wasKing = piece->getIsKing();
		piece->setIsKing(treatAsKing);
		bool canKeepJumping = (hadJumpedDuringMove && canMovePieceAt(previousCoord, piece, true));
		piece->setIsKing(wasKing);

		if (canKeepJumping)
			return "If jumping, the piece cannot stop jumping until there are no more jumps available";

		piece->setIsKing(treatAsKing);
		checkerBoard_->removeAt(startCoord);
		checkerBoard_->setPiece(previousCoord, piece);

		return nullptr;
	}


	const char * Game::attemptMove(const Move & move)
	{
		std::vector<CompactCoordinate> toBeRemoved = std::vector<CompactCoordinate>();
		const char * error = attemptMoveInternal(move, &toBeRemoved);

		for (unsigned int i = 0; i < toBeRemoved.size(); i++)
		{
			// Reset mark from in progress captures
			checkerBoard_->getPiece(toBeRemoved[i])->setMark(0);

			// If move was valid, remove taken pieces
			if (error == nullptr)
			{
				CheckerPiece *piece = checkerBoard_->removeAt(toBeRemoved[i]);
			}
		}
		return error;
	}

	bool Game::checkForWinCondition(int playerIndex) const
	{
		Player *otherPlayer = players_[(playerIndex + 1) % numPlayers_];
		return (checkerBoard_->getNumPieces(otherPlayer->getControllingSide()) == 0 || !canAnyPieceMove(otherPlayer->getControllingSide()));
	}

	int Game::run()
	{
		
		bool gameIsRunning = true;

		while (gameIsRunning)
		{
			std::cout << *checkerBoard_ << std::endl;

			bool validMoveReceived = false;
			while (!validMoveReceived)
			{
				Move move = players_[currentPlayerTurn_]->requestMove();

				std::cout << "Trying move: " << move << std::endl;

				// Validate move
				CheckerPiece *piece = checkerBoard_->getPiece(move.getCoordinate(0));
				if (piece == nullptr || piece->getSide() != players_[currentPlayerTurn_]->getControllingSide())
				{
					// Not a valid checker
					std::cout << "Not a valid target. Checker is not at given position or it does not belong to you." << std::endl;
				}
				else
				{
					const char * error = attemptMove(move);
					validMoveReceived = error == nullptr;

					if (!validMoveReceived)
					{
						std::cout << "Not a valid move: " << error << std::endl;
					}
				}
			}
			
			if (checkForWinCondition(currentPlayerTurn_))
			{
				gameIsRunning = false;
			}
			else
			{
				currentPlayerTurn_ = (currentPlayerTurn_ + 1) % numPlayers_;
			}

			std::cout << std::endl << std::endl;
		}

		std::cout << *checkerBoard_;
		std::cout << "Player '" << players_[currentPlayerTurn_]->getSymbol() << "' wins!" << std::endl;

		return currentPlayerTurn_;
	}

	const Player * Game::getPlayer(int index) const
	{
		return players_[index];
	}

}