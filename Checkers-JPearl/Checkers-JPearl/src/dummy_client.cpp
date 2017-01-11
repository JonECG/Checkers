#include "dummy_client.h"

#include <iostream>
#include <string>

#include "connection.h"

namespace checkers
{
	const char * DummyClient::kDefaultPort = "32123";

	void DummyClient::run()
	{
		std::cout << "Enter Host Address (default \"localhost\") > ";
		std::string host = std::string();
		std::getline(std::cin, host);
		if (host.empty())
			host = "localhost";

		std::cout << "Enter Port (default " << kDefaultPort << ") > ";
		std::string port = std::string();
		std::getline(std::cin, port);
		if (port.empty())
			port = kDefaultPort;

		Connection conn;
		if (Connection::connectTo(host.c_str(), port.c_str(), conn))
		{
			while (conn.isConnected())
			{
				if (conn.waitUntilHasMessage())
				{
					checkers::MessageType type;
					const char * data = nullptr;
					unsigned int length;

					data = conn.processMessage(type, length);

					if (data != nullptr)
					{
						switch (type)
						{
						case MessageType::SEND_MESSAGE:
							std::cout << data << std::flush;
							break;
						case MessageType::REQUEST_INPUT:
							std::string response = std::string();
							std::getline(std::cin, response);
							conn.sendMessage(response);
							break;
						}
					}
				}
			}
			std::cout << "Connection to server terminated " << std::endl;
		}
		else
		{
			std::cout << "Couldn't connect" << std::endl;
			printSockError("");
		}
	}
}
