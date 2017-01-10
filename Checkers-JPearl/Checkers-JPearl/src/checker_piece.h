#pragma once

namespace checkers
{
	enum PieceSide : unsigned char
	{
		O = 0,
		X = 1,
	};

	class CheckerPiece
	{
		bool isKing_ : 1;
		PieceSide side_ : 1;
		unsigned char mark_ : 6;
	public:
		CheckerPiece();

		bool getIsKing() const;
		void setIsKing(bool isKing);

		PieceSide getSide() const;
		void setSide(PieceSide side);

		unsigned char getMark() const;
		void setMark(unsigned char mark);

		// Returns the symbol that represents the side of this piece and whether it has been crowned king
		char getSymbol() const;
	};
}
