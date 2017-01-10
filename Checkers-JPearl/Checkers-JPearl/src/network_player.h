#pragma once

#include "player.h"

namespace checkers
{
	class Connection;
	class NetworkPlayer : public Player
	{
		Connection *connection_;
	public:
		NetworkPlayer(Connection *connection);
		const char * getDescriptor() const override;
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}
