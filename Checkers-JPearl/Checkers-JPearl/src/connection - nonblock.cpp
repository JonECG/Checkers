#include "connection.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

// Using a mix of WinSock and BSD interface, some defines here to keep things clean below
#ifdef _WIN32
	#define errno WSAGetLastError()
#else
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define closesocket close
	#define WSAEINPROGRESS EINPROGRESS
#endif

#define BLOCKING 0
#define NONBLOCKING 1

namespace checkers
{
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

	int setBlockingStateOnSocket(SOCKET sock, unsigned long blockState)
	{
		return ioctlsocket(sock, FIONBIO, &blockState);
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

	int setBlockingStateOnSocket(SOCKET sock, unsigned long blockState)
	{
		// BSD sockets are file descriptors, have to do extra stuff to toggle access mode
		int accessMode = fcntl(sock, F_GETFL, 0);
		if (accessMode < 0)
			return SOCKET_ERROR;

		if (blockState == BLOCKING)
			accessMode &= ~O_NONBLOCK;
		if (blockState == NONBLOCKING )
			accessMode |= O_NONBLOCK;

		return ( fcntl( sock, F_SETFL, blockState ) == 0 ) ? 0 : SOCKET_ERROR;
	}
#endif

	bool Connection::listenTo(unsigned short port, Connection &outConnection, unsigned int timeout, const char ** error)
	{
		if (!isInit_)
			init();
		
		SOCKADDR_IN address;
		int addressSize = sizeof(address);

		SOCKET sockListen, sockConnection;

		// Simple reliable TCP connection
		sockConnection = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (sockConnection == INVALID_SOCKET)
		{
			if (error != nullptr)
				*error = strerror(errno);
			return false;
		}

		address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		address.sin_family = AF_INET;
		address.sin_port = htons(port);

		sockListen = socket(AF_INET, SOCK_STREAM, NULL);
		if (sockListen == INVALID_SOCKET)
		{
			if (error != nullptr)
				*error = strerror(errno);
			closesocket(sockConnection);
			return false;
		}

		bind(sockListen, (SOCKADDR*)&address, sizeof(address));
		
		listen(sockListen, SOMAXCONN) == 0;

		//while (session->isActive)
		{
			sockConnection = accept(sockListen, (SOCKADDR*)&address, &addressSize);
			if (sockConnection)
			{
				bool isSuccessful = send(sockConnection, "Hi.", 4, NULL);
			}
		}

		return false;
	}

	bool Connection::connectTo(std::string ip, unsigned short port, Connection &outConnection, unsigned int timeout, const char ** error)
	{
		if (!isInit_)
			init();

		SOCKET sock;
		// Simple reliable TCP connection
		sock = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
		{
			if(error != nullptr)
				*error = strerror(errno);
			return false;
		}

		setBlockingStateOnSocket(sock, NONBLOCKING);

		SOCKADDR_IN address;
		address.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
		address.sin_family = AF_INET;
		address.sin_port = htons(port);

		bool success = false;
		if (connect(sock, (SOCKADDR*)&address, sizeof(address)) != SOCKET_ERROR)
		{
			success = true;
		}
		else
		{
			if (errno == WSAEINPROGRESS)
			{
				struct timeval tv;
				fd_set rfds;
				FD_ZERO(&rfds);
				FD_SET(sock, &rfds);

				tv.tv_sec = 0;
				tv.tv_usec = (long)timeout * 1000;
			}
		}

		if (success)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sock;
			outConnection.isHosting_ = false;
			return true;
		}

		if (error != nullptr)
			*error = strerror(errno);
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

	bool Connection::isHosting() const
	{
		return isHosting_;
	}

}