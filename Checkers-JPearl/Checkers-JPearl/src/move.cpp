#include "move.h"

#include <stdexcept>

#include "checker_board.h"

namespace checkers
{
	Move::Move()
	{
		numCoords_ = 0;
	}
	void Move::addCoordinate(CompactCoordinate coord)
	{
		if (numCoords_ < kMaxCoordsPerMove)
			moveCoords_[numCoords_++] = coord;
	}
	unsigned char Move::getNumCoords() const
	{
		return numCoords_;
	}

	CompactCoordinate Move::getCoordinate(int index) const
	{
		if (index < 0 || index > numCoords_)
			throw std::out_of_range("Index out of range or object has already been released");
		return moveCoords_[index];
	}

	bool Move::isForfeit() const
	{
		return *reinterpret_cast<unsigned const char *>(moveCoords_ + ( kMaxCoordsPerMove - 1 ) ) == kForfeitValue;
	}

	void Move::makeForfeit()
	{
		*reinterpret_cast<unsigned char *>(moveCoords_ + (kMaxCoordsPerMove - 1)) = kForfeitValue;
	}

	Move Move::parseFromString(const char * sequence)
	{
		Move result = Move();

		int currentColumn = -1;
		
		int index = 0;
		while (sequence[index] != '\0')
		{
			char currentChar = sequence[index];

			// Whitespace
			if (currentChar == ' ')
			{
				// Actually, just be whitespace agnostic

				//if (currentColumnChar != -1)
				//{
				//	// We were expecting a row immediately after
				//	throw std::invalid_argument("Could not parse move. Coordinate was interupted by whitespace");
				//}
			}
			else
			// Column
			if ((currentChar >= 'a' && currentChar < 'a' + CheckerBoard::kNumColumns) || (currentChar >= 'A' && currentChar < 'A' + CheckerBoard::kNumColumns))
			{
				bool isUpperCase = (currentChar >= 'A' && currentChar < 'A' + CheckerBoard::kNumColumns);

				if( currentColumn != -1)
					throw std::invalid_argument("Could not parse move. Coordinate row was expected after column but another column index was provided");

				if(result.numCoords_ == Move::kMaxCoordsPerMove)
					throw std::invalid_argument("Could not parse move. Too many coordinates.");

				currentColumn = (isUpperCase) ? currentChar - 'A' : currentChar - 'a';
			}
			else
			// Row
			if (currentChar >= '1' && currentChar < '1' + CheckerBoard::kNumRows)
			{
				if (currentColumn == -1)
					throw std::invalid_argument("Could not parse move. Coordinate column was expected first but row index was provided");

				int currentRow = currentChar - '1';
				result.moveCoords_[result.numCoords_].column = currentColumn;
				result.moveCoords_[result.numCoords_].row = currentRow;
				result.numCoords_++;
				currentColumn = -1;
			}
			else
			// Anything else
			{
				throw std::invalid_argument("Could not parse move. Unexpected symbol encountered");
			}

			index++;
		}

		if (currentColumn != -1)
			throw std::invalid_argument("Could not parse move. String ended unexpectedly");

		if (result.numCoords_ < 2)
			throw std::invalid_argument("Could not parse move. Move must contain at least two sets of coordinates");

		return result;
	}

	std::ostream & operator<<(std::ostream & stream, const Move & move)
	{
		if (move.isForfeit())
			return stream << "FORFEIT";


		for (int i = 0; i < move.numCoords_; i++)
		{
			stream << (char)('A' + move.moveCoords_[i].column) << (char)('1' + move.moveCoords_[i].row);

			if(i < move.numCoords_-1)
				stream << " ";
		}
		return stream;
	}
}
