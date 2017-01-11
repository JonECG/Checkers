#pragma once
#ifndef CHECKER_BOARD_H
#define CHECKER_BOARD_H

#include <ostream>

#include "compact_coordinate.h"

namespace checkers
{
	class CheckerPiece;
	class Move;
	enum PieceSide : unsigned char;

	class CheckerBoard
	{
	public:
		static const int kNumRows = 8;
		static const int kNumActualColumns = 1; // How many playable columns there are per row
		static const int kNumRowsPerPlayer = 3;
		static const int kNumSides = 2;

		static const int kNumColumns = kNumActualColumns * kNumSides;
		static const int kNumPiecesPerPlayer = kNumRowsPerPlayer * kNumActualColumns;
		static const int kNumPieces = kNumSides * kNumPiecesPerPlayer;
		static const int kNumCells = kNumRows * kNumActualColumns;

	private:
		CheckerPiece* pieces_;
		CheckerPiece* board_[kNumCells];
		unsigned short pieceCount_[kNumSides];

		bool isRowShifted(int row) const;
		int getIndexFromCoord(CompactCoordinate coord) const;
		CheckerPiece* getPiece(int index) const;

	public:
		// Allocates memory for pieces, also calls setupBoard()
		void initialize();
		// Allocates memory for pieces and clones state from given board and its pieces
		void initialize(const CheckerBoard& toClone);
		// Releases memory allocated by initialize()
		void release();

		// Clears board state and resets pieces
		void setupBoard();

		bool isCoordValid(CompactCoordinate coord) const;

		// Returns the number of pieces for the given side on the board
		unsigned short getNumPieces(PieceSide side) const;

		// Returns the piece at the given coordinates. Returns null if no piece is there or coordinate is invalid
		CheckerPiece* getPiece(CompactCoordinate coord) const;

		// Sets a piece at the given coordinates. Returns whether it was successful. Will fail with invalid coordinates such as trying to get the index for a coordinate that is between playable spaces.
		bool setPiece(CompactCoordinate coord, CheckerPiece* piece);

		// Removes any piece that was at the coordinate and returns it. If coordinate is invalid, or no piece is present, nullptr is returned
		CheckerPiece* removeAt(CompactCoordinate coord);

		// Inserts a textual representation of the board and its pieces into a stream
		friend std::ostream& operator<< (std::ostream& stream, const CheckerBoard& board);
	};
}

#endif // CHECKER_BOARD_H
