#pragma once

#include "player.h"
namespace checkers
{
	class Game;
	class AiPlayer : public Player
	{
		Game *game_;
		int recurseLevels_;
	public:
		AiPlayer(Game * game, int recurseLevels);

		Move requestMove() const override;
	};
}

