#pragma once

#include "checker_board.h"

namespace checkers
{
	class Player;
	class Game
	{
		CheckerBoard checkerBoard_;
		Player *players_;
		unsigned char numPlayers_;
		unsigned char currentPlayerTurn_;

		// Attempt the move and updates board state, and will not perform the move if it is not valid. Returns whether the move was successfully performed.
		bool attemptMove(const Move& move);
	public:
		Game();

		void initialize();
		void release();

		// The main game loop plays through the game and returns the index of the player that won
		int run();

		const Player* getPlayer(int index) const;
	};
}