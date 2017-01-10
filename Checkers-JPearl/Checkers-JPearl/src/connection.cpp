#include "connection.h"

#include <thread>

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
#else
	#include <string.h>
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
#ifdef _WIN32
	#define AddressLength int
#else
	#define AddressLength unsigned int
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
	void setAddressIp( SOCKADDR_IN &address, const char * ip )
	{
		address.sin_addr.S_un.S_addr = inet_addr(ip);
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
	void setAddressIp( SOCKADDR_IN &address, const char * ip )
	{
		inet_aton(ip, &address.sin_addr);
	}
#endif

	void Connection::run()
	{
		std::thread runningThread = std::thread([this] {runLoop(); });
		runningThread.detach();
	}

	void Connection::runLoop()
	{
		while (isConnected_)
		{
			// As long as we haven't wrapped fully around the circular buffer
			while (idxQueuedMessagesStart_ != (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages)
			{
				char * packet = queuedMessages_ + kMaxMessageSize * idxQueuedMessagesEnd_;
				if (recv(socket_, packet, kMaxMessageSize, 0) != SOCKET_ERROR)
				{
					idxQueuedMessagesEnd_ = (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages;
				}
			}
		}
		closesocket(socket_);
	}

	bool Connection::listenTo(unsigned short port, Connection &outConnection, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets
 
		if (!isInit_)
			init();

		SOCKET sockListen;

		// Simple reliable TCP connection
		sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockListen == INVALID_SOCKET)
		{
			// Clean up
			return false;
		}

		SOCKADDR_IN address;
		AddressLength addressSize = sizeof(address);
 
		setAddressIp( address, "127.0.0.1" );
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
 
		int reuseAddress = 1;
		setsockopt( sockListen, SOL_SOCKET, SO_REUSEADDR, (char*) &reuseAddress, sizeof reuseAddress );

		if (bind(sockListen, (SOCKADDR*)&address, addressSize) == SOCKET_ERROR)
		{
			// Clean up
			closesocket(sockListen);
			return false;
		}
		
		if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR)
		{
			// Clean up
			closesocket(sockListen);
			return false;
		}

		SOCKET sockIncomingConnection = accept(sockListen, (SOCKADDR*)&address, &addressSize);
		if (sockIncomingConnection != INVALID_SOCKET)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sockIncomingConnection;
			outConnection.isHosting_ = true;
			outConnection.run();
			closesocket(sockListen);
			return true;
		}
		
		// Clean up
		closesocket(sockListen);

		return false;
	}

	bool Connection::connectTo(std::string ip, unsigned short port, Connection &outConnection, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets

		if (!isInit_)
			init();
 
		SOCKET sock;
		// Simple reliable TCP connection
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
		{
			return false;
		}
 
		SOCKADDR_IN address;
		setAddressIp(address, ip.c_str());
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
			outConnection.run();
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
		// Type		CHAR	1
		// Length	SHORT	2 hton
		// Message	CHAR*	X

		if (data == nullptr)
			length = 0;

		if (length + 3 > kMaxMessageSize)
			return false; // Too long of a message

		char buffer[kMaxMessageSize];

		int index = 0;

		buffer[index++] = type;
		*(reinterpret_cast<unsigned short*>(buffer+index)) = htons( (unsigned short) length );
		index += 2;

		for (unsigned int i = 0; i < length; i++)
		{
			buffer[index++] = data[i];
		}

		if (send(socket_, buffer, index, 0) == SOCKET_ERROR)
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
		return idxQueuedMessagesStart_ != idxQueuedMessagesEnd_;
	}
	
	const char * Connection::processMessage(MessageType & outType, unsigned int & outLength)
	{
		if (hasMessageWaiting())
		{
			char * buffer = queuedMessages_ + kMaxMessageSize * idxQueuedMessagesStart_;
			outType = (MessageType)buffer[0];
			outLength = ntohs(*reinterpret_cast<unsigned short*>(buffer + 1));
			memcpy_s(currentMessage_, kMaxMessageSize, buffer, outLength + 3);
			
			idxQueuedMessagesStart_ = (idxQueuedMessagesStart_ + 1) % kMaxNumberOfMessages;
			return currentMessage_ + 3;
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
