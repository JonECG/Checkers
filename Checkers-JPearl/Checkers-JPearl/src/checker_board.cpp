#include "checker_board.h"

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
