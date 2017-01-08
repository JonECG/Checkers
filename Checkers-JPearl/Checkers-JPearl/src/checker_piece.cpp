#include "checker_piece.h"

namespace checkers
{
	CheckerPiece::CheckerPiece()
	{
		isKing_ = false;
		mark_ = 0;
	}

	bool CheckerPiece::getIsKing() const
	{
		return isKing_;
	}

	void CheckerPiece::setIsKing(bool isKing)
	{
		isKing_ = isKing;
	}

	PieceSide CheckerPiece::getSide() const
	{
		return side_;
	}

	void CheckerPiece::setSide(PieceSide side)
	{
		side_ = side;
	}
	unsigned char CheckerPiece::getMark() const
	{
		return mark_;
	}
	void CheckerPiece::setMark(unsigned char mark)
	{
		mark_ = mark;
	}
	char CheckerPiece::getSymbol() const
	{
		// Avoiding nested ifs by switching on the two binary values combined
		switch (side_ << 1 | (int)isKing_)
		{
		case 0: // 00
			return 'o';
		case 1: // 01
			return 'O';
		case 2: // 10
			return 'x';
		case 3: // 11
			return 'X';
		}
		return '?';
	}
}