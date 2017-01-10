#pragma once

#include "player.h"
namespace checkers
{
	class LocalPlayer : public Player
	{
		Game *game_;
	public:
		LocalPlayer(Game *game);
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}
