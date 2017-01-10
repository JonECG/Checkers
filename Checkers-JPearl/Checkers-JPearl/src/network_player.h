#pragma once

#include "player.h"

namespace checkers
{
	class Connection;
	class NetworkPlayer : public Player
	{
		Game *game_;
		Connection *connection_;
	public:
		NetworkPlayer(Game *game, Connection *connection);
		const char * getDescriptor() const override;
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}
