#include "game.h"

#include <iostream>
#include <vector>

#include "player.h"
#include "ai_player.h"
#include "local_player.h"
#include "move.h"

namespace checkers
{
	Game::Game(bool echoMessagesToConsole)
	{
		currentPlayerTurn_ = 0;
		echoMessagesToConsole_ = echoMessagesToConsole;
	}

	void Game::initialize()
	{
		checkerBoard_ = new CheckerBoard();
		checkerBoard_->initialize();

		for (int i = 0; i < kNumPlayers; i++)
		{
			if (!players_[i])
				players_[i] = new LocalPlayer();

			players_[i]->setControllingSide((PieceSide)i);
			players_[i]->setGame(this);
		}
	}

	void Game::release()
	{
		checkerBoard_->release();
		delete checkerBoard_;

		for (int i = 0; i < kNumPlayers; i++)
		{
			if (players_[i])
				delete players_[i];
		}
	}

	std::ostream& Game::messageWriter()
	{
		return currentMessage_;
	}

	void Game::sendMessageToPlayers(bool excludeCurrent)
	{
		for (int i = 0; i < kNumPlayers; i++)
		{
			if(!excludeCurrent || currentPlayerTurn_ != i)
				players_[i]->sendMessage(currentMessage_.str().c_str());
		}

		if (echoMessagesToConsole_)
			std::cout << currentMessage_.str() << std::flush;

		currentMessage_.str(std::string());
	}

	void Game::registerPlayer(Player * player, PieceSide side)
	{
		if (players_[side])
			delete players_[side];

		players_[side] = player;
		players_[side]->setControllingSide(side);
		players_[side]->setGame(this);
	}

	void Game::runMoves(char ** moves, int numMoves)
	{
		for (int i = 0; i < numMoves; i++)
		{
			try
			{
				Move move = Move::parseFromString(moves[i]);
				attemptMove(move);
				currentPlayerTurn_ = (currentPlayerTurn_ + 1) % kNumPlayers;
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
			char deltaX = (direction & 0b01) ? 1 : -1;
			char deltaY = (direction & 0b10) ? 1 : -1;

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

	bool Game::canAnyPieceMove(PieceSide side, bool onlyJumpMoves, bool ignoreMarked, int * count) const
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
						{
							if (count)
								(*count)++;
							else
								return true; // Early out
						}
					}
				}
			}
		}
		return (count != nullptr && *count > 0);
	}

	void Game::jumpExplorationRecursion(const Move & moveToExplore, CheckerPiece * target, Move * moves, int& outCurrentIndex) const
	{
		CompactCoordinate endCoord = moveToExplore.getCoordinate(moveToExplore.getNumCoords() - 1);
		CompactCoordinate middleCoord = moveToExplore.getCoordinate(moveToExplore.getNumCoords() - 2);
		middleCoord.column = (middleCoord.column + endCoord.column) / 2;
		middleCoord.row = (middleCoord.row + endCoord.row) / 2;

		CheckerPiece *jumpedPiece = checkerBoard_->getPiece(middleCoord);
		unsigned char prevMark = jumpedPiece->getMark();
		jumpedPiece->setMark(1);

		bool wasKing = target->getIsKing();
		if ((target->getSide() == PieceSide::X && endCoord.row == 0) || (target->getSide() == PieceSide::O && endCoord.row == CheckerBoard::kNumRows - 1))
			target->setIsKing(true);

		const int maxMovesPerPiece = 4;
		CompactCoordinate results[maxMovesPerPiece];
		int numResults = maxMovesPerPiece;

		canMovePieceAt(endCoord, target, true, true, results, &numResults);

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

			char deltaX = currentCoord.column - previousCoord.column;
			char deltaY = currentCoord.row - previousCoord.row;

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


	const char * Game::attemptTurn(const Move & move)
	{
		CheckerPiece *piece = checkerBoard_->getPiece(move.getCoordinate(0));
		// Validate move
		if (move.getNumCoords() < 2)
		{
			return "Not enough coordinates to be a move.";
		}
		else
		if (piece == nullptr || piece->getSide() != players_[currentPlayerTurn_]->getControllingSide())
		{
			// Not a valid checker
			return "Missing target. There is no checker at the given position or it does not belong to you.";
		}
		else
		{
			return attemptMove(move);
		}
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
				checkerBoard_->removeAt(toBeRemoved[i]);
			}
		}
		return error;
	}

	bool Game::checkForWinCondition(int playerIndex) const
	{
		Player *otherPlayer = players_[(playerIndex + 1) % kNumPlayers];
		return (checkerBoard_->getNumPieces(otherPlayer->getControllingSide()) == 0 || !canAnyPieceMove(otherPlayer->getControllingSide()));
	}

	void Game::writeAllMovesAvailable(PieceSide side)
	{
		Move moves[Game::kMoveArraySize];
		int startIndex = 0;
		int count = findAllMoves(side, moves, Game::kMoveArraySize, startIndex);

		messageWriter() << "Available Moves: \n";
		for (int i = 0; i < count; i++)
		{
			messageWriter() << "\t" << (i + 1) << ") " << moves[i + startIndex] << '\n';
		}
	}

	int Game::run()
	{
		bool gameIsRunning = true;

		while (gameIsRunning)
		{
			messageWriter() << *checkerBoard_;

			bool validMoveReceived = false;
			while (!validMoveReceived)
			{
				// Prints available moves
				messageWriter() << "Enter a move as {start} {destination1} {destinationX...}  Eg: c3 d4 or e3 c5 e7\n";
				writeAllMovesAvailable(players_[currentPlayerTurn_]->getControllingSide());
				messageWriter() << "You can also forfeit by typing 'FORFEIT'\n";

				// Request move from player 
				messageWriter() << players_[currentPlayerTurn_]->getDescriptor() << "Player '" << players_[currentPlayerTurn_]->getSymbol() << "'> ";
				sendMessageToPlayers();
				players_[currentPlayerTurn_]->sendMessage("[YOU] > ");
				Move move = players_[currentPlayerTurn_]->requestMove();

				// Display current move
				messageWriter() << move << " -- Move provided\n";

				// Check for forfeit
				if (move.isForfeit())
				{
					messageWriter() << players_[currentPlayerTurn_]->getDescriptor() << "Player '" << players_[currentPlayerTurn_]->getSymbol() << "' forfeits...\n";
					gameIsRunning = false;
					break;
				}

				// Attempt to take turn with the move
				const char * error = attemptTurn(move);
				if (error)
					messageWriter() << "Not a valid move. " << error << '\n';
				else
					validMoveReceived = true;
				sendMessageToPlayers();
			}
			
			if (checkForWinCondition(currentPlayerTurn_))
			{
				gameIsRunning = false;
			}
			else
			{
				currentPlayerTurn_ = (currentPlayerTurn_ + 1) % kNumPlayers;
			}

			messageWriter() << "\n\n\n";
		}

		// Write out final board state
		messageWriter() << *checkerBoard_;
		messageWriter() << players_[currentPlayerTurn_]->getDescriptor() << "Player '" << players_[currentPlayerTurn_]->getSymbol() << "' wins!\n";
		sendMessageToPlayers();
		return currentPlayerTurn_;
	}

	int Game::findAllMoves(PieceSide side, Move * moves, int moveCapacity, int& outStartPosition) const
	{
		CheckerBoard *cb = checkerBoard_;

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
						canMovePieceAt(coord, piece, false, false, results, &numResults);

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

	const Player * Game::getPlayer(int index) const
	{
		return players_[index];
	}

}
