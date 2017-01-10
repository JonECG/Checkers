#pragma once

#include "checker_board.h"

namespace checkers
{
	class Player;
	enum PieceSide : unsigned char;
	class Game
	{
		friend class AiPlayer;

		static const int kNumPlayers = 2;

		CheckerBoard *checkerBoard_;
		Player *players_[kNumPlayers];
		unsigned char currentPlayerTurn_;

		// Returns whether a piece has any valid moves from a given position. If no piece is given, it runs the check on the piece at the given position returning false if no piece is there. Can also restrict to only consider jump moves.
		// If given an array of coordinates and its, will return all spaces the piece can move to and update the numCoordinates to reflect how many were returned
		bool canMovePieceAt(CompactCoordinate coord, CheckerPiece *piece = nullptr, bool onlyJumpMoves = false, bool ignoreMarked = true, CompactCoordinate * coordinates = nullptr, int * numCoordinates = nullptr) const;
		// Returns whether there are any pieces that can move on the given side. Can also restrict to only consider jump moves. If given a pointer to an int, it will count the number of pieces that can move
		bool canAnyPieceMove(PieceSide side, bool onlyJumpMoves = false, bool ignoreMarked = true, int * count = nullptr) const;
		// Used to traverse all jump paths, storing them in the moves array and updating outCurrentIndex to the current first availabe spot
		void jumpExplorationRecursion(const Move& moveToExplore, CheckerPiece * target, Move * moves, int& outCurrentIndex) const;
		// Attempt the move and updates board state, and will not perform the move if it is not valid. Returns whether the move was successfully performed. If given a pointer to a cstring pointer, will point it to an error message if it occurs
		const char * attemptMove(const Move& move);
		const char * attemptMoveInternal(const Move& move, void * data);
		// Returns whether the current player has won
		bool checkForWinCondition(int playerIndex) const;
	public:
		static const int kMaxMovesPerPiece = 4;
		static const int kMoveArraySize = CheckerBoard::kNumPiecesPerPlayer*kMaxMovesPerPiece;

		Game();

		void initialize();
		void release();

		// Transfers ownership of player to game to use. Game will then take over freeing memory of the player in release() or when another player is registerred to that spot
		void registerPlayer(Player *player, PieceSide side);

		// For debug -- will run moves
		void runMoves(char ** moves, int numMoves);

		// The main game loop plays through the game and returns the index of the player that won
		int run();

		// Find all valid moves for the given side, returns the number of moves available and stored starting from the index returned in outStartPosition
		int findAllMoves(PieceSide side, Move * moves, int moveCapacity, int& outStartPosition) const;

		const Player* getPlayer(int index) const;
	};
}
