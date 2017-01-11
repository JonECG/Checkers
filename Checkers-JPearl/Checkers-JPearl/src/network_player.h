#pragma once
#ifndef NETWORK_PLAYER_H
#define NETWORK_PLAYER_H

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

#endif // NETWORK_PLAYER_H
