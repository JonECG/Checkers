#pragma once

#include "player.h"
namespace checkers
{
	class LocalPlayer : public Player
	{
	public:
		Move requestMove() override;
	};
}