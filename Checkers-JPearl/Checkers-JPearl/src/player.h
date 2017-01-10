#pragma once

#include "checker_piece.h"
namespace checkers
{
	class Move;
	class Game;
	class Player
	{
		Game *game_;
		PieceSide controllingSide_;
	public:
		virtual ~Player();

		// Gets the descriptor for this player to be shown
		virtual const char * getDescriptor() const = 0;

		// Requests a move from the player
		virtual Move requestMove() = 0;

		// Sends a message to the player
		virtual void sendMessage(const char * message) const = 0;

		// Returns the symbol that represents the side this player controls
		char getSymbol() const;

		PieceSide getControllingSide() const;
		void setControllingSide(PieceSide side);

		Game* getGame() const;
		void setGame(Game *game);
	};
} 
