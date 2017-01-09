#pragma once

#include <ostream>
#include "compact_coordinate.h"

namespace checkers
{
	class Move
	{
		// As incredibly unlikely it is to capture all 12 of the enemy's pieces in a single move, it's the theoretical maximum
		static const int kMaxCoordsPerMove = 13;
		unsigned char numCoords_;
		CompactCoordinate moveCoords_[kMaxCoordsPerMove];
		
	public:
		Move();

		void addCoordinate(CompactCoordinate coord);

		unsigned char getNumCoords() const;
		CompactCoordinate getCoordinate(int index) const;

		// Parses a string such as d6 f4 d2 into a move with CompactCoordinates. Excepts if the data is invalid
		static Move parseFromString(const char* sequence);

		// Inserts a textual representation of the move coordinates into a stream
		friend std::ostream& operator<< (std::ostream& stream, const Move& move);
	};
}