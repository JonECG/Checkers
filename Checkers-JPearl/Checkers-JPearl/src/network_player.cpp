#include "network_player.h"

#include <sstream>

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
		Move result;

		bool moveSuccessfullyMade = false;

		while (!moveSuccessfullyMade)
		{
			
			std::string input;
			connection_->requestInput(input);

			try
			{
				if (!connection_->isConnected() || input.find("FORFEIT") != std::string::npos || input.find("forfeit") != std::string::npos)
					result.makeForfeit();
				else
					result = checkers::Move::parseFromString(input.c_str());
				moveSuccessfullyMade = true;
			}
			catch (std::exception& e)
			{
				std::ostringstream os = std::ostringstream();
				os << "Invalid formatting: " << e.what() << "\nTry again > ";
				connection_->sendMessage(os.str());
			}
		}
		return result;
	}
	void NetworkPlayer::sendMessage(const char * message) const
	{
		connection_->sendMessage(message);
	}
}
