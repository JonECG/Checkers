#pragma once

#include "checker_board.h"

namespace checkers
{
	class Player;
	enum PieceSide : unsigned char;
	class Game
	{
		CheckerBoard checkerBoard_;
		Player *players_;
		unsigned char numPlayers_;
		unsigned char currentPlayerTurn_;

		// Returns whether a piece has any valid moves from a given position. If no piece is given, it runs the check on the piece at the given position returning false if no piece is there. Can also restrict to only consider jump moves
		bool canMovePieceAt(CompactCoordinate coord, CheckerPiece *piece = nullptr, bool onlyJumpMoves = false, bool ignoreMarked = true) const;
		// Returns whether there are any pieces that can move on the given side. Can also restrict to only consider jump moves
		bool canAnyPieceMove(PieceSide side, bool onlyJumpMoves = false, bool ignoreMarked = true) const;
		// Attempt the move and updates board state, and will not perform the move if it is not valid. Returns whether the move was successfully performed. If given a pointer to a cstring pointer, will point it to an error message if it occurs
		const char * attemptMove(const Move& move);
		const char * attemptMoveInternal(const Move& move, void * data);
		// Returns whether the current player has won
		bool checkForWinCondition();
	public:
		Game();

		void initialize();
		void release();

		// For debug -- will run moves
		void runMoves(char ** moves, int numMoves);

		// The main game loop plays through the game and returns the index of the player that won
		int run();

		const Player* getPlayer(int index) const;
	};
}