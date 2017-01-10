#include "connection.h"

#include <thread>
#include <iostream>

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
	int Connection::getLastError(char * buffer, int bufferLength)
	{
		int wsaError = WSAGetLastError();

		if (buffer)
		{
			LPSTR errString = NULL;
			int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, wsaError, 0, (LPSTR)&errString, 0, 0);
			memcpy_s(buffer, bufferLength, errString, size + 1);
			LocalFree(errString);
		}

		return wsaError;
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
	int Connection::getLastError(char * buffer, int bufferLength)
	{
		int errorId = errno;

		if (buffer)
		{
			const char * error = strerror(errorId);
			for (int i = 0; i < bufferLength - 1; i++)
			{
				buffer[i] = error[i];
				if (error[i] == '\0')
					return;
			}
			buffer[bufferLength - 1] = '\0';
		}

		return errorId;
	}
	void setAddressIp( SOCKADDR_IN &address, const char * ip )
	{
		inet_aton(ip, &address.sin_addr);
	}
#endif

	Connection::Connection()
	{
		idxQueuedMessagesStart_ = 0;
		idxQueuedMessagesEnd_ = 0;
	}

	void Connection::run()
	{
		idxQueuedMessagesStart_ = 0;
		idxQueuedMessagesEnd_ = 0;
		std::thread runningThread = std::thread([this] {runLoop(); });
		runningThread.detach();
	}

	void Connection::runLoop()
	{
		while (isConnected_)
		{
			// As long as we haven't wrapped fully around the circular buffer
			if (idxQueuedMessagesStart_ != (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages)
			{
				char * packet = queuedMessages_ + kMaxMessageSize * idxQueuedMessagesEnd_;

				bool success = false;
				int meta = 3;
				if (recv(socket_, packet, meta, 0) != SOCKET_ERROR)
				{
					unsigned short length = ntohs(*reinterpret_cast<unsigned short*>(packet + 1));
					if (length == 0 || recv(socket_, packet + meta, length, 0) != SOCKET_ERROR)
					{
						processMutex.lock();
						idxQueuedMessagesEnd_ = (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages;
						processMutex.unlock();
						success = true;
					}
				}

				if( !success )
				{
					if (getLastError() == WSAECONNRESET)
						isConnected_ = false;

					printSockError("Error hosting : " << isHosting_ << " on receive");
				}
			}
		}
		disconnect();
	}

	bool Connection::listenTo(unsigned short port, ConnectionListener &outListener, unsigned int timeout)
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
			printSockError("Error creating listen socket");
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
			printSockError("Error binding listen socket");
			closesocket(sockListen);
			return false;
		}
		
		if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR)
		{
			// Clean up
			printSockError("Error listening listen socket");
			closesocket(sockListen);
			return false;
		}

		outListener.socket_ = sockListen;
		outListener.isListening_ = true;
		
		return true;
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
			printSockError("Error creating connecting socket");
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

		printSockError("Error connecting");

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

	bool Connection::sendPayload(MessageType type, const char * data, unsigned int length)
	{
		// Type		CHAR	1
		// Length	SHORT	2 hton
		// Message	CHAR*	X

		if (!isConnected_)
			return false;

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

		sendMutex.lock();
		int result = send(socket_, buffer, index, 0);
		sendMutex.unlock();

		if (result == SOCKET_ERROR)
		{
			printSockError("Error hosting : " << isHosting_ << " on send payload");
			return false;
		}

		return true;
	}

	bool Connection::sendMessage(std::string message)
	{
		return sendPayload(MessageType::SEND_MESSAGE, message.c_str(), message.length() + 1);
	}

	bool Connection::requestInput(std::string &outResponse)
	{
		if (!isHosting_)
			return false; // Can't request input from host

		if (sendPayload(MessageType::REQUEST_INPUT))
		{
			if (waitUntilHasMessage())
			{
				MessageType type;
				unsigned int length;

				const char * message = processMessage(type, length);

				if (message != nullptr)
				{
					outResponse = std::string(message);
					return true;
				}
			}
		}

		return false;
	}

	bool Connection::waitUntilHasMessage() const
	{
		while (isConnected_ && !hasMessageWaiting()) { std::this_thread::yield(); }
		return hasMessageWaiting();
	}

	bool Connection::hasMessageWaiting() const
	{
		return idxQueuedMessagesStart_ != idxQueuedMessagesEnd_;
	}
	
	const char * Connection::processMessage(MessageType & outType, unsigned int & outLength)
	{
		processMutex.lock();
		const char * result = nullptr;
		if (hasMessageWaiting())
		{
			char * buffer = queuedMessages_ + kMaxMessageSize * idxQueuedMessagesStart_;
			outType = (MessageType)buffer[0];
			outLength = ntohs(*reinterpret_cast<unsigned short*>(buffer + 1));
			memcpy_s(currentMessage_, kMaxMessageSize, buffer, outLength + 3);
			
			idxQueuedMessagesStart_ = (idxQueuedMessagesStart_ + 1) % kMaxNumberOfMessages;
			result = currentMessage_ + 3;
		}
		processMutex.unlock();
		return result;
	}

	bool Connection::isConnected() const
	{
		return isConnected_;
	}

	bool Connection::isHosting() const
	{
		return isHosting_;
	}

	void ConnectionListener::end()
	{
		if (isListening_)
		{
			closesocket(socket_);
			isListening_ = false;
		}
	}

	bool ConnectionListener::isListening() const
	{
		return isListening_;
	}

	bool ConnectionListener::acceptConnection(Connection & outConnection, unsigned int timeout)
	{
		timeout;

		SOCKADDR_IN address;
		AddressLength addressSize = sizeof(address);

		SOCKET sockIncomingConnection = accept(socket_, (SOCKADDR*)&address, &addressSize);
		if (sockIncomingConnection != INVALID_SOCKET)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sockIncomingConnection;
			outConnection.isHosting_ = true;
			outConnection.run();
			return true;
		}
		else
		{
			end();
			return false;
		}
	}

}
