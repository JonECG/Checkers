#include "checker_board.h"

#include "checker_piece.h"

namespace checkers
{
	// Returns whether the row given should be shifted over to make up for the compressed column representation
	bool CheckerBoard::isRowShifted(int row) const
	{
		return row & 1;
	}
	// Returns the index into the board_ array that corresponds to the given coordinates. Excepts on invalid coordinates such as trying to get the index for a coordinate that is between playable spaces
	int CheckerBoard::getIndexFromCoord(CompactCoordinate coord) const
	{
		// TODO: account for shifting and except on nonplay coords
		return coord.column / 2 + coord.row * kNumColumns;
	}

	void CheckerBoard::initialize()
	{
		pieces_ = new CheckerPiece[kNumPieces];

		setupBoard();
	}

	void CheckerBoard::release()
	{
		delete[] pieces_;
		pieces_ = nullptr;
	}

	void CheckerBoard::setupBoard()
	{
		int piecesPerPlayer = kNumPieces / 2;

		// Team O
		for (int i = 0; i < piecesPerPlayer; i++)
		{
			pieces_[i].setIsKing(false);
			pieces_[i].setSide(PieceSide::O);
			board_[i] = pieces_ + i;
		}

		// Clear middle rows
		int startMiddleRowsIndex = kNumColumns * kNumRowsPerPlayer;
		int endMiddleRowsIndex = kNumColumns * (kNumRows - kNumRowsPerPlayer);
		for (int i = startMiddleRowsIndex; i < endMiddleRowsIndex; i++)
		{
			board_[i] = nullptr;
		}

		// Team X
		for (int i = piecesPerPlayer; i < kNumPieces; i++)
		{
			pieces_[i].setIsKing(false);
			pieces_[i].setSide(PieceSide::X);
			board_[kNumCells + piecesPerPlayer - i - 1 ] = pieces_ + i;
		}
	}

	CheckerPiece* CheckerBoard::getPiece(int index) const
	{
		return board_[index];
	}
	CheckerPiece* CheckerBoard::getPiece(CompactCoordinate coord) const
	{
		return getPiece(getIndexFromCoord(coord));
	}


	std::ostream & operator<<(std::ostream & stream, const CheckerBoard & board)
	{
		for (int y = CheckerBoard::kNumRows; y >= -1; y--)
		{
			bool isPaddedRow = (y == -1 || y == CheckerBoard::kNumRows);

			if (isPaddedRow)
			{
				stream << ' ';
				for (int x = 0; x < CheckerBoard::kNumColumns * 2; x++)
				{
					stream << (char)('a' + x);
				}
				stream << ' ';
			}
			else
			{
				stream << (char)('1' + y);
				for (int x = 0; x < CheckerBoard::kNumColumns * 2; x++)
				{
					if (!board.isRowShifted(y) ^ (x & 1) )
					{
						CheckerPiece* piece = board.getPiece(x / 2 + y * CheckerBoard::kNumColumns);
						if (piece)
						{
							stream << piece->getSymbol();
						}
						else
						{
							stream << '.';
						}
					}
					else
					{
						stream << ' ';
					}
				}
				stream << (char)('1' + y);
			}
			
			stream << std::endl;
		}

		return stream;
	}
}
