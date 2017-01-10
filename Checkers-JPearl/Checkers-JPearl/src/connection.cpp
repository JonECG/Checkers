#include "connection.h"


#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/ip.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <iostream>
	#include <unistd.h> 
#endif

// Using WinSock interface, some defines here to keep things clean below
#ifndef _WIN32
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define closesocket close
	#define WSAEINPROGRESS EINPROGRESS
	#define SOCKADDR_IN sockaddr_in
	#define SOCKET int // Sockets are just file descriptors in BSD
	#define SOCKADDR sockaddr
#endif

#define BLOCKING 0
#define NONBLOCKING 1

namespace checkers
{
	bool Connection::isInit_ = false;
#ifdef _WIN32
	void Connection::init()
	{
		if (!isInit_)
		{
			WORD versionWanted = MAKEWORD(2, 1);
			WSADATA wsaData;
			int isSuccessful = WSAStartup(versionWanted, &wsaData);
			if (isSuccessful)
				isInit_ = true;
		}
	}

	void Connection::shutdown()
	{
		if (isInit_)
		{
			int isSuccessful = WSACleanup();
			if (isSuccessful)
				isInit_ = false;
		}
	}
	void Connection::getLastError(char * buffer, int bufferLength)
	{
		strerror_s(buffer, bufferLength, errno);
	}
#else
	void Connection::init()
	{
		isInit_ = true;
	}

	void Connection::shutdown()
	{
		isInit_ = false;
	}
	void Connection::getLastError(char * buffer, int bufferLength)
	{
		const char * error = strerror(errno);
		for (int i = 0; i < bufferLength - 1; i++)
		{
			buffer[i] = error[i];
			if (error[i] == '\0')
				return;
		}
		buffer[bufferLength - 1] = '\0';
	}
#endif

	bool Connection::listenTo(unsigned short port, Connection &outConnection, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets

		if (!isInit_)
			init();
		
		SOCKADDR_IN address;
		int addressSize = sizeof(address);

		SOCKET sockListen, sockConnection;

		// Simple reliable TCP connection
		sockConnection = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (sockConnection == INVALID_SOCKET)
		{
			return false;
		}

		address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		address.sin_family = AF_INET;
		address.sin_port = htons(port);

		sockListen = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (sockListen == INVALID_SOCKET)
		{
			// Clean up
			closesocket(sockConnection);
			return false;
		}

		if (bind(sockListen, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR)
		{
			// Clean up
			closesocket(sockListen);
			closesocket(sockConnection);
			return false;
		}
		
		if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR)
		{
			// Clean up
			closesocket(sockListen);
			closesocket(sockConnection);
			return false;
		}


		SOCKET sockIncomingConnection = accept(sockListen, (SOCKADDR*)&address, &addressSize);
		if (sockConnection != INVALID_SOCKET)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sockIncomingConnection;
			outConnection.isHosting_ = true;
			return true;
		}
		
		// Clean up
		closesocket(sockListen);
		closesocket(sockConnection);

		return false;
	}

	bool Connection::connectTo(std::string ip, unsigned short port, Connection &outConnection, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets

		if (!isInit_)
			init();

		SOCKET sock;
		// Simple reliable TCP connection
		sock = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
		{
			return false;
		}

		SOCKADDR_IN address;
		address.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
		address.sin_family = AF_INET;
		address.sin_port = htons(port);

		bool success = false;
		if (connect(sock, (SOCKADDR*)&address, sizeof(address)) != SOCKET_ERROR)
		{
			success = true;
		}

		if (success)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sock;
			outConnection.isHosting_ = false;
			return true;
		}

		// Clean up
		closesocket(sock);

		return false;
	}

	void Connection::disconnect()
	{
		if (isConnected_)
		{
			closesocket(socket_);
			isConnected_ = false;
		}
	}

	bool Connection::sendPayload(MessageType type, const char * data, unsigned int length) const
	{
		// Type		CHAR
		// Length	CHAR
		// Message	CHAR*

		if (data == nullptr)
			length = 0;

		if (length + 2 > kMaxMessageSize)
			return false; // Too long of a message

		char buffer[kMaxMessageSize];

		int index = 0;

		buffer[index++] = type;
		buffer[index++] = (char) length;
		for (unsigned int i = 0; i < length; i++)
		{
			buffer[index++] = data[i];
		}

		if (send(socket_, buffer, index, NULL) == SOCKET_ERROR)
		{
			return false;
		}

		return true;
	}

	bool Connection::sendMessage(std::string message) const
	{
		return sendPayload(MessageType::SEND_MESSAGE, message.c_str(), message.length() + 1);
	}

	bool Connection::requestInput(std::string &outResponse)
	{
		if (!isHosting_)
			return false; // Can't request input from host

		if (sendPayload(MessageType::REQUEST_INPUT))
		{
			while (!hasMessageWaiting());

			MessageType type;
			unsigned int length;

			const char * message = processMessage(type, length);

			if (message != nullptr)
			{
				outResponse = std::string(message);
				return true;
			}
		}

		return false;
	}

	bool Connection::hasMessageWaiting()
	{
		if (!isMessageReady_)
		{
			if (recv(socket_, currentMessage_, kMaxMessageSize, NULL) != SOCKET_ERROR)
			{
				isMessageReady_ = true;
			}
		}

		return isMessageReady_;
	}

	const char * Connection::processMessage(MessageType & outType, unsigned int & outLength)
	{
		if (isMessageReady_)
		{
			outType = (MessageType) currentMessage_[0];
			outLength = (unsigned int) currentMessage_[1];
			isMessageReady_ = false;
			return currentMessage_ + 2;
		}
		return nullptr;
	}

	bool Connection::isConnected() const
	{
		return isConnected_;
	}

	bool Connection::isHosting() const
	{
		return isHosting_;
	}

}
