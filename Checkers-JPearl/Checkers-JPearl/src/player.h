#pragma once

#include "checker_piece.h"
namespace checkers
{
	class Move;
	class Player
	{
		int numPiecesRemaining_;
		PieceSide controllingSide_;
	public:

		// Requests a move from the player
		Move requestMove() const;

		int getNumPiecesRemaining() const;
		void setNumPiecesRemaining(int numPieces);
		PieceSide getControllingSide() const;
		void setControllingSide(PieceSide side);
	};
} 