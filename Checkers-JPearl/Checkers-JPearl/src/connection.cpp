#include "connection.h"

namespace checkers
{

	bool Connection::listen(int port, Connection &outConnection, unsigned int timeout)
	{
		outConnection.isConnected_ = true;
		return true;
	}

	void Connection::connect(std::string ip, int port)
	{
		if (!isConnected_)
			isConnected_ = true;
	}

	void Connection::disconnect()
	{
		if (isConnected_)
			isConnected_ = false;
	}

	void Connection::sendMessage(std::string message) const
	{
	}

	std::string Connection::requestInput()
	{
		return std::string();
	}

	bool Connection::isConnected() const
	{
		return isConnected_;
	}

}