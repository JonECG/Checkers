#include "checker_board.h"

#include <stdexcept>
#include <vector>

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

	bool CheckerBoard::isCoordValid(CompactCoordinate coord) const
	{
		bool rowShifted = isRowShifted(coord.row);

		return !(coord.column & 1 ^ rowShifted);
	}

	// All of the hard work of move validation happens here
	bool CheckerBoard::attemptMove(const Move & move)
	{
		CompactCoordinate startCoord = move.getCoordinate(0);
		int startIndex = getIndexFromCoord(startCoord);
		
		if (startIndex == -1)
			return false; // Start position was not valid

		CheckerPiece *piece = board_[startIndex];

		if (!piece)
			return false; // No checker in start position

		// Board and piece state do not change until the move is completed
		bool treatAsKing = piece->getIsKing();
		std::vector<unsigned char> toBeRemoved = std::vector<unsigned char>();

		CompactCoordinate previousCoord = startCoord;
		for (int i = 1; i < move.getNumCoords(); i++)
		{
			CompactCoordinate currentCoord = move.getCoordinate(i);
			int index = getIndexFromCoord(currentCoord);

			if (index == -1)
				return false; // Intermediate position was not valid

			int deltaX = currentCoord.column - previousCoord.column;
			int deltaY = currentCoord.row - previousCoord.row;

			if (std::abs(deltaX) != std::abs(deltaY))
				return false; // Checkers can only move diagonally

			int distance = std::abs(deltaY);

			switch (distance)
			{
			case 0:
				return false; // Checkers have to move at least one space
			case 1:

				if (move.getNumCoords() > 2)
					return false; // Moves to adjacent squares can only be made in a singular move

				if (!treatAsKing && ( (deltaY > 0) ^ (piece->getSide() == PieceSide::O) ))
					return false; // This piece cannot move that way

				if (board_[index] != nullptr)
					return false; // The space is already occupied

				break;
			case 2:
			{
				if (!treatAsKing && ((deltaY > 0) ^ (piece->getSide() == PieceSide::O)))
					return false; // This piece cannot move that way

				if (board_[index] != nullptr)
					return false; // The space is already occupied

				CompactCoordinate middleCoord = previousCoord;
				middleCoord.column += deltaX / 2;
				middleCoord.row += deltaY / 2;
				unsigned char middleIndex = getIndexFromCoord(middleCoord);

				if (board_[middleIndex] == nullptr || board_[middleIndex]->getSide() == piece->getSide())
					return false; // Can't jump over empty spaces or pieces of your color

				toBeRemoved.push_back(middleIndex);

				break;
			}
			default:
				return false; // Checkers can't move that far
			}


			// Evaluate if piece should be king from this step
			if ( (piece->getSide() == PieceSide::X && currentCoord.row == 0) || (piece->getSide() == PieceSide::O && currentCoord.row == kNumRows - 1) )
				treatAsKing = true;

			previousCoord = currentCoord;
		}


		// If move was valid, apply the move
		for (unsigned int i = 0; i < toBeRemoved.size(); i++)
		{
			board_[toBeRemoved[i]] = nullptr;
		}
		piece->setIsKing(treatAsKing);
		board_[startIndex] = nullptr;
		board_[getIndexFromCoord(previousCoord)] = piece;
		return true;
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
