#include "game.h"

#include <iostream>
#include <vector>

#include "player.h"
#include "move.h"

namespace checkers
{
	Game::Game()
	{
		currentPlayerTurn_ = 0;
	}

	void Game::initialize()
	{
		checkerBoard_.initialize();
		players_ = new Player[2];
		numPlayers_ = 2;

		players_[PieceSide::O].setControllingSide(PieceSide::O);
		players_[PieceSide::X].setControllingSide(PieceSide::X);
	}

	void Game::release()
	{
		checkerBoard_.release();
		delete[] players_;
	}

	// All of the hard work of move validation happens here
	bool Game::attemptMove(const Move & move)
	{
		CompactCoordinate startCoord = move.getCoordinate(0);

		if (!checkerBoard_.isCoordValid(startCoord))
			return false; // Start position was not valid

		CheckerPiece *piece = checkerBoard_.getPiece(startCoord);

		if (!piece)
			return false; // No checker in start position

						  // Board and piece state do not change until the move is completed
		bool treatAsKing = piece->getIsKing();
		std::vector<CompactCoordinate> toBeRemoved = std::vector<CompactCoordinate>();

		CompactCoordinate previousCoord = startCoord;
		for (int i = 1; i < move.getNumCoords(); i++)
		{
			CompactCoordinate currentCoord = move.getCoordinate(i);
			
			if (!checkerBoard_.isCoordValid(currentCoord))
				return false; // Intermediate position was not valid

			int deltaX = currentCoord.column - previousCoord.column;
			int deltaY = currentCoord.row - previousCoord.row;

			if (std::abs(deltaX) != std::abs(deltaY))
				return false; // Checkers can only move diagonally

			int distance = std::abs(deltaY);

			switch (distance)
			{
			case 0:
				return false; // Checkers have to move at least one space
			case 1:

				if (move.getNumCoords() > 2)
					return false; // Moves to adjacent squares can only be made in a singular move

				if (!treatAsKing && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
					return false; // This piece cannot move that way

				if (checkerBoard_.getPiece(currentCoord) != nullptr)
					return false; // The space is already occupied

				break;
			case 2:
			{
				if (!treatAsKing && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
					return false; // This piece cannot move that way

				if (checkerBoard_.getPiece(currentCoord) != nullptr)
					return false; // The space is already occupied

				CompactCoordinate middleCoord = previousCoord;
				middleCoord.column += deltaX / 2;
				middleCoord.row += deltaY / 2;
				CheckerPiece *middlePiece = checkerBoard_.getPiece(middleCoord);

				if (middlePiece == nullptr || middlePiece->getSide() == piece->getSide())
					return false; // Can't jump over empty spaces or pieces of your color

				toBeRemoved.push_back(middleCoord);

				break;
			}
			default:
				return false; // Checkers can't move that far
			}


			// Evaluate if piece should be king from this step
			if ((piece->getSide() == PieceSide::X && currentCoord.row == 0) || (piece->getSide() == PieceSide::O && currentCoord.row == CheckerBoard::kNumRows - 1))
				treatAsKing = true;

			previousCoord = currentCoord;
		}


		// If move was valid, apply the move
		for (unsigned int i = 0; i < toBeRemoved.size(); i++)
		{
			checkerBoard_.removeAt(toBeRemoved[i]);
		}
		piece->setIsKing(treatAsKing);
		checkerBoard_.removeAt(startCoord);
		checkerBoard_.setPiece(previousCoord, piece);
		return true;
	}

	int Game::run()
	{
		
		bool gameIsRunning = true;

		while (gameIsRunning)
		{
			std::cout << checkerBoard_;

			bool validMoveReceived = false;
			while (!validMoveReceived)
			{
				Move move = players_[currentPlayerTurn_].requestMove();

				// Validate move
				CheckerPiece *piece = checkerBoard_.getPiece(move.getCoordinate(0));
				if (piece == nullptr || piece->getSide() != players_[currentPlayerTurn_].getControllingSide())
				{
					// Not a valid checker
					std::cout << "Not a valid target. Checker is not at given position or it does not belong to you." << std::endl;
				}
				else
				{
					validMoveReceived = attemptMove(move);

					if (!validMoveReceived)
					{
						std::cout << "Not a valid move" << std::endl;
					}
				}
			}
			
			currentPlayerTurn_ = (currentPlayerTurn_ + 1) % numPlayers_;

			std::cout << std::endl << std::endl;
		}

		return 0;
	}

	const Player * Game::getPlayer(int index) const
	{
		return &(players_[index]);
	}

}