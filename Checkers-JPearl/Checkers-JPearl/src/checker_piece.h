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
		bool isKing_;
		PieceSide side_;
	public:
		CheckerPiece();

		bool getIsKing() const;
		void setIsKing(bool isKing);

		PieceSide getSide() const;
		void setSide(PieceSide side);

		// Returns the symbol that represents the side of this piece and whether it has been crowned king
		char getSymbol() const;
	};
}