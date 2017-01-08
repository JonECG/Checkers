#include "move.h"

#include <stdexcept>

namespace checkers
{

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

	Move Move::parseFromString(const char * sequence)
	{
		Move result = Move();

		int currentColumn = -1;
		
		int index = 0;
		while (sequence[index] != '\0')
		{
			char currentChar = sequence[index];
			bool isUpperCase = false;

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
			if ((currentChar >= 'a' && currentChar <= 'h') || ( isUpperCase = (currentChar >= 'A' && currentChar <= 'H' )))
			{
				if( currentColumn != -1)
					throw std::invalid_argument("Could not parse move. Coordinate row was expected after column but another column index was provided");

				if(result.numCoords_ == Move::kMaxCoordsPerMove)
					throw std::invalid_argument("Could not parse move. Too many coordinates.");

				currentColumn = (isUpperCase) ? currentChar - 'A' : currentChar - 'a';
			}
			else
			// Row
			if (currentChar >= '1' && currentChar <= '8')
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

		return result;
	}

}