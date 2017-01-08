#pragma once

#include <ostream>

#include "compact_coordinate.h"

namespace checkers
{
	class CheckerPiece;
	class Move;

	class CheckerBoard
	{
	public:
		static const int kNumRows = 8;
		static const int kNumColumns = 4;
		static const int kNumRowsPerPlayer = 3;

		static const int kNumPieces = 2 * kNumRowsPerPlayer * kNumColumns;
		static const int kNumCells = kNumRows * kNumColumns;

	private:
		CheckerPiece* pieces_;
		CheckerPiece* board_[kNumCells];

		bool isRowShifted(int row) const;
		int getIndexFromCoord(CompactCoordinate coord) const;
		CheckerPiece* getPiece(int index) const;

	public:
		// Allocates memory for pieces, also calls setupBoard()
		void initialize();
		// Releases memory allocated by initialize()
		void release();

		// Clears board state and resets pieces
		void setupBoard();

		bool isCoordValid(CompactCoordinate coord) const;

		// Returns the piece at the given coordinates. Returns null if no piece is there or coordinate is invalid
		CheckerPiece* getPiece(CompactCoordinate coord) const;

		// Sets a piece at the given coordinates. Returns whether it was successful. Will fail with invalid coordinates such as trying to get the index for a coordinate that is between playable spaces.
		bool setPiece(CompactCoordinate coord, CheckerPiece* piece);

		// Removes any piece that was at the coordinate and returns it. If coordinate is invalid, or no piece is present, nullptr is returned
		CheckerPiece* removeAt(CompactCoordinate coord);

		// Inserts a textual representation of the board and its pieces into a stream
		friend std::ostream& operator<< (std::ostream& stream, const CheckerBoard& matrix);
	};
}