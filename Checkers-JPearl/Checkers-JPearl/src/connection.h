#pragma once

#include <string>

namespace checkers
{
	class Connection
	{
		bool isConnected_;
	public:
		// Listens on the port given for any incoming connection and will write it to the given outConnection parameter. Returns whether a connection was found before timeout (in milliseconds)
		static bool listen(int port, Connection &outConnection, unsigned int timeout = 1000);

		void connect(std::string ip, int port);

		void disconnect();

		// Sends a message to the other end
		void sendMessage(std::string message) const;

		// Requests a message from the other end
		std::string requestInput();

		bool isConnected() const;
	};
}