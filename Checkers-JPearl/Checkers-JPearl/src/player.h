#pragma once

#include "checker_piece.h"
namespace checkers
{
	class Move;
	class Player
	{
		PieceSide controllingSide_;
	public:

		// Requests a move from the player
		Move requestMove() const;

		// Returns the symbol that represents the side this player controls
		char getSymbol() const;
		PieceSide getControllingSide() const;
		void setControllingSide(PieceSide side);
	};
} 