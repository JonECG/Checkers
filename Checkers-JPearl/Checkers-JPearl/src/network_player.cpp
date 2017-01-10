#include "network_player.h"

#include "connection.h"
#include "move.h"
namespace checkers
{
	NetworkPlayer::NetworkPlayer(Game * game, Connection *connection)
	{
		game_ = game;
		connection_ = connection;
	}

	Move NetworkPlayer::requestMove()
	{
		return Move();
	}
	void NetworkPlayer::sendMessage(const char * message) const
	{
		connection_->sendMessage(message);
	}
}
