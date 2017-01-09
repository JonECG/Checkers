#include "player.h"

namespace checkers
{
	Player::~Player()
	{
	}

	char Player::getSymbol() const
	{
		return (controllingSide_ == PieceSide::O) ? 'o' : 'x';
	}

	PieceSide Player::getControllingSide() const
	{
		return controllingSide_;
	}

	void Player::setControllingSide(PieceSide side)
	{
		controllingSide_ = side;
	}

}