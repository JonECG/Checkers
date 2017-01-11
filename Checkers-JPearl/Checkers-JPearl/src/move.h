#pragma once
#ifndef MOVE_H
#define MOVE_H

#include <ostream>
#include "compact_coordinate.h"

namespace checkers
{
	class Move
	{
		// As incredibly unlikely it is to capture all 12 of the enemy's pieces in a single move, it's the theoretical maximum
		static const int kMaxCoordsPerMove = 13;

		// What does it mean? Probably everything. But really this is just a number to inject to signify a move as being forfeit
		static const unsigned char kForfeitValue = 42;
		
		unsigned char numCoords_;
		CompactCoordinate moveCoords_[kMaxCoordsPerMove];
		
	public:
		Move();

		void addCoordinate(CompactCoordinate coord);

		unsigned char getNumCoords() const;
		CompactCoordinate getCoordinate(int index) const;

		// Returns whether this move represents a forfeit
		bool isForfeit() const;
		// Turns this move into a forfeit
		void makeForfeit();

		// Parses a string such as d6 f4 d2 into a move with CompactCoordinates. Excepts if the data is invalid
		static Move parseFromString(const char* sequence);

		// Inserts a textual representation of the move coordinates into a stream
		friend std::ostream& operator<< (std::ostream& stream, const Move& move);
	};
}

#endif // MOVE_H
