#pragma once

#include <ostream>

#include "compact_coordinate.h"

namespace checkers
{
	class CheckerPiece;

	class CheckerBoard
	{
		static const int kNumRows = 8;
		static const int kNumColumns = 4;
		static const int kNumRowsPerPlayer = 3;

		static const int kNumPieces = 2 * kNumRowsPerPlayer * kNumColumns;
		static const int kNumCells = kNumRows * kNumColumns;

		CheckerPiece* pieces_;
		CheckerPiece* board_[kNumCells];

		bool isRowShifted(int row) const;
		int getIndexFromCoord(CompactCoord coord) const;
		CheckerPiece* getPiece(int index) const;

	public:
		// Allocates memory for pieces, also calls setupBoard()
		void initialize();
		// Releases memory allocated by initialize()
		void release();

		// Clears board state and resets pieces
		void setupBoard();

		// Returns the piece at the given coordinates. Returns null if no piece is there. Excepts on invalid coordinates such as trying to get the index for a coordinate that is between playable spaces.
		CheckerPiece* getPiece(CompactCoord coord) const;

		// Inserts a textual representation of the board and its pieces into a stream
		friend std::ostream& operator<< (std::ostream& stream, const CheckerBoard& matrix);
	};
}