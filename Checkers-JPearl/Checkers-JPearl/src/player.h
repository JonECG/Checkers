#pragma once

#include "checker_piece.h"
namespace checkers
{
	class Move;
	class Player
	{
		PieceSide controllingSide_;
	public:
		virtual ~Player();

		// Requests a move from the player
		virtual Move requestMove() const = 0;

		// Returns the symbol that represents the side this player controls
		char getSymbol() const;
		PieceSide getControllingSide() const;
		void setControllingSide(PieceSide side);
	};
} 