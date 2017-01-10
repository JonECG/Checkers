#include "dummy_client.h"

#include <iostream>
#include <string>

#include "connection.h"

namespace checkers
{
	void DummyClient::run()
	{
		std::cout << "Enter IP (default 127.0.0.1) > ";
		std::string ip = std::string();
		std::getline(std::cin, ip);
		if (ip.empty())
			ip = "127.0.0.1";

		std::cout << "Enter Port (default " << kDefaultPort << ") > ";
		std::string portString = std::string();
		std::getline(std::cin, portString);
		unsigned short port = kDefaultPort;

		try
		{
			if (!portString.empty())
				port = (unsigned short)std::stoi(portString);
		}
		catch (std::exception)
		{
			std::cout << "Couldn't parse port string, using " << kDefaultPort << std::endl;
		}

		Connection conn;
		if (Connection::connectTo(ip, port, conn))
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
		}
	}
}
