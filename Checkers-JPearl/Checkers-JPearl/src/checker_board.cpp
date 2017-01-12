#include "checker_board.h"

#include <climits>

#include "checker_piece.h"
#include "move.h"

namespace checkers
{
	// Returns whether the row given should be shifted over to make up for the compressed column representation
	bool CheckerBoard::isRowShifted(int row) const
	{
		return row & 1;
	}
	// Returns the index into the board_ array that corresponds to the given coordinates. Returns -1 on invalid coordinates such as trying to get the index for a coordinate that is between playable spaces
	int CheckerBoard::getIndexFromCoord(CompactCoordinate coord) const
	{
		bool rowShifted = isRowShifted(coord.row);

		if (coord.column & 1 ^ rowShifted)
			return -1;

		return coord.column / 2 + coord.row * kNumActualColumns;
	}

	void CheckerBoard::initialize()
	{
		pieces_ = new CheckerPiece[kNumPieces];

		setupBoard();
	}

	void CheckerBoard::initialize(const CheckerBoard & toClone)
	{
		pieces_ = new CheckerPiece[kNumPieces];
		for (int i = 0; i < kNumPieces; i++)
		{
			pieces_[i] = toClone.pieces_[i];
		}

		for (int i = 0; i < kNumCells; i++)
		{
			if (toClone.board_[i] == nullptr)
			{
				board_[i] = nullptr;
			}
			else
			{
				// Some pointer arithmetic to find the index into the array 
				int offset = toClone.board_[i] - toClone.pieces_;
				board_[i] = pieces_ + offset;
			}
		}

		pieceCount_[0] = toClone.pieceCount_[0];
		pieceCount_[1] = toClone.pieceCount_[1];
	}

	void CheckerBoard::release()
	{
		delete[] pieces_;
		pieces_ = nullptr;
	}

	void CheckerBoard::setupBoard()
	{
		// Team O
		for (int i = 0; i < kNumPiecesPerPlayer; i++)
		{
			pieces_[i].setIsKing(false);
			pieces_[i].setMark(0);
			pieces_[i].setSide(PieceSide::O);
			board_[i] = pieces_ + i;
		}

		// Clear middle rows
		int startMiddleRowsIndex = kNumActualColumns * kNumRowsPerPlayer;
		int endMiddleRowsIndex = kNumActualColumns * (kNumRows - kNumRowsPerPlayer);
		for (int i = startMiddleRowsIndex; i < endMiddleRowsIndex; i++)
		{
			board_[i] = nullptr;
		}

		// Team X
		for (int i = kNumPiecesPerPlayer; i < kNumPieces; i++)
		{
			pieces_[i].setIsKing(false);
			pieces_[i].setMark(0);
			pieces_[i].setSide(PieceSide::X);
			board_[kNumCells + kNumPiecesPerPlayer - i - 1 ] = pieces_ + i;
		}

		pieceCount_[0] = kNumPiecesPerPlayer;
		pieceCount_[1] = kNumPiecesPerPlayer;
	}

	bool CheckerBoard::isCoordValid(CompactCoordinate coord) const
	{
		bool rowShifted = isRowShifted(coord.row);

		return !(coord.column & 1 ^ rowShifted) && coord.column >= 0 && coord.column < kNumColumns && coord.row >= 0 && coord.row < kNumRows;
	}

	unsigned short CheckerBoard::getNumPieces(PieceSide side) const
	{
		return pieceCount_[side];
	}


	CheckerPiece* CheckerBoard::getPiece(int index) const
	{
		return board_[index];
	}
	CheckerPiece* CheckerBoard::getPiece(CompactCoordinate coord) const
	{
		return getPiece(getIndexFromCoord(coord));
	}

	bool CheckerBoard::setPiece(CompactCoordinate coord, CheckerPiece* piece)
	{
		int index = getIndexFromCoord(coord);
		if (index == -1)
			return false;

		CheckerPiece *previousPiece = board_[index];
		if (previousPiece)
			pieceCount_[previousPiece->getSide()]--;
		board_[index] = piece;
		if (piece)
			pieceCount_[piece->getSide()]++;
		return true;
	}

	CheckerPiece* CheckerBoard::removeAt(CompactCoordinate coord)
	{
		int index = getIndexFromCoord(coord);
		if (index == -1)
			return nullptr;

		CheckerPiece *piece = board_[index];
		if (piece)
			pieceCount_[piece->getSide()]--;
		board_[index] = nullptr;
		return piece;
	}

	unsigned char getCeiledLog2(unsigned int value)
	{
		unsigned char result = 0;
		while (value > 0)
		{
			value >>= 1;
			result++;
		}
		return result;
	}

	uint_least64_t CheckerBoard::currentBoardState() const
	{
		int_least64_t result = 0;
		unsigned char currentBit = 0;

		/*
			Represents the board as follows:
				For each available square write the following
					if is empty
						write 0
					else
						write 1
						write side 
			
			A board will have at maximum
				4 * 8 = 32 squares
				4 * 3 * 2 = 24 pieces
				The maximum board will take up ( 24 ) * 2 + ( 32 - 24 ) * 1 = 56 bits

			In the last 8 bits we write
				num O kings in 4 bits
				num X kings in 4 bits

			With this we can represent a board in 64 bits
			the only missing information being which pieces are kings but should be good enough to not be encountered in a game
			And only works if it's a default board
		*/

		// Only perform the masking if we can fit in 64 bits
		unsigned char numBits = sizeof result * CHAR_BIT;
		unsigned char numBitsForKingsPerPlayer = getCeiledLog2(kNumPiecesPerPlayer);
		unsigned char numBitsRequired = kNumPieces * 2 + (kNumCells - kNumPieces) + numBitsForKingsPerPlayer * 2;
		if (numBitsRequired <= numBits )
		{
			unsigned char numKings[2];
			const uint_least64_t kOne = 1;
			for (int i = 0; i < kNumCells; i++)
			{
				CheckerPiece *piece = getPiece(i);
				if (!piece)
				{
					currentBit++;
				}
				else
				{
					result |= kOne << currentBit++;
					result |= (piece->getSide() & kOne) << currentBit++;
					if (piece->getIsKing())
						numKings[piece->getSide() & kOne]++;
				}
			}
			for (int i = 0; i < 2; i++)
			{
				for (int bit = 0; bit < numBitsForKingsPerPlayer; bit++)
				{
					result |= ( (numKings[i] >> bit) & kOne) << currentBit++;
				}
			}
		}

		return result;
	}


	std::ostream & operator<<(std::ostream & stream, const CheckerBoard & board)
	{
		for (int y = CheckerBoard::kNumRows; y >= -1; y--)
		{
			bool isPaddedRow = (y == -1 || y == CheckerBoard::kNumRows);

			if (isPaddedRow)
			{
				stream << "  ";
				for (int x = 0; x < CheckerBoard::kNumActualColumns * 2; x++)
				{
					// Show uppercase values on top and lowercase on bottom
					stream << (char)(((y == -1) ? 'a' : 'A' ) + x) << ' ';
				}
				stream << "  ";
			}
			else
			{
				stream << (char)('1' + y) << ' ';
				for (int x = 0; x < CheckerBoard::kNumActualColumns * 2; x++)
				{
					if (!board.isRowShifted(y) ^ (x & 1) )
					{
						CheckerPiece* piece = board.getPiece(x / 2 + y * CheckerBoard::kNumActualColumns);
						if (piece)
						{
							stream << piece->getSymbol() << ' ';
						}
						else
						{
							stream << ". ";
						}
					}
					else
					{
						stream << "  ";
					}
				}
				stream << (char)('1' + y) << ' ';
			}
			
			stream << '\n';
		}

		return stream;
	}
}
