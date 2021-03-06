#include "connection.h"

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <WinSock2.h>
	#include <Ws2ipdef.h>
	#include <Ws2tcpip.h>
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

#include <thread>
#include <iostream>
#include <chrono>

// Using WinSock interface, some defines here to keep things clean below
#ifdef _WIN32
	#define AddressLength int
	#define SHUT_RD 0
	#define SHUT_WR 1
	#define SHUT_RDWR 2
#else
	#define AddressLength unsigned int
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define closesocket close
	#define WSAEINPROGRESS EINPROGRESS
	#define SOCKADDR_IN sockaddr_in
	#define SOCKET int // Sockets are just file descriptors in BSD
	#define SOCKADDR sockaddr
	#define WSAECONNRESET ECONNRESET
	
#endif

#ifdef DEBUG
	#define verboseInfo(message) std::cout << "\n" << message << std::endl;
	#ifdef _WIN32
		#define setLastError( message ) { lastError_ = WSAGetLastError(); Connection::connectionErrorMessage_ = message; }
	#else // _WIN32
		#define setLastError( message ) { lastError_ = errno; Connection::connectionErrorMessage_ = message; }
	#endif // _WIN32
#else // DEBUG
	#define verboseInfo(message)
	#define setLastError( message )
#endif // DEBUG

#define BLOCKING 0
#define NONBLOCKING 1

namespace checkers
{
	bool Connection::isInit_ = false;
	int Connection::lastError_ = 0;
	const char * Connection::connectionErrorMessage_ = nullptr;
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

	void Connection::cleanup()
	{
		if (isInit_)
		{
			int isSuccessful = WSACleanup();
			if (isSuccessful)
				isInit_ = false;
		}
	}
	int Connection::getLastError(char * buffer, int bufferLength, const char ** outCustomMessage)
	{
		int wsaError = lastError_;

		if (outCustomMessage)
		{
			*outCustomMessage = connectionErrorMessage_;
		}
		if (buffer)
		{
			LPSTR errString = NULL;
			int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, wsaError, 0, (LPSTR)&errString, 0, 0);
			memcpy_s(buffer, bufferLength, errString, size + 1);
			LocalFree(errString);
		}

		return wsaError;
	}
#else
	void Connection::init()
	{
		isInit_ = true;
	}

	void Connection::cleanup()
	{
		isInit_ = false;
	}
	int Connection::getLastError(char * buffer, int bufferLength, const char ** outCustomMessage)
	{
		int errorId = lastError_;

		if (outCustomMessage)
		{
			*outCustomMessage = connectionErrorMessage_;
		}
		if (buffer)
		{
			const char * error = strerror(errorId);
			for (int i = 0; i < bufferLength - 1; i++)
			{
				buffer[i] = error[i];
				if (error[i] == '\0')
					return errorId;
			}
			buffer[bufferLength - 1] = '\0';
		}

		return errorId;
	}
#endif

	Connection::Connection()
	{
		waitingForAck_ = false;
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
		while (isConnected_ || waitingForAck_)
		{
			// As long as we haven't wrapped fully around the circular buffer
			if (idxQueuedMessagesStart_ != (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages)
			{
				char * packet = queuedMessages_ + kMaxMessageSize * idxQueuedMessagesEnd_;

				bool success = false;
				int meta = 3;
				int bytesReceived = recv(socket_, packet, meta, 0);
				if (bytesReceived != SOCKET_ERROR && (bytesReceived != 0 || meta == 0))
				{
					unsigned char type = *reinterpret_cast<unsigned char*>(packet);
					if (type == MessageType::FIN && isConnected_)
					{
						verboseInfo("raw received FIN packet");
						sendPayload(checkers::MessageType::FINACK);
						disconnect(true);
						std::thread ackTimeOut = std::thread([this] {
							std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
							std::chrono::steady_clock::time_point endTime = startTime + std::chrono::milliseconds(kAckTimeoutMilliseconds);

							while (waitingForAck_ && std::chrono::steady_clock::now() < endTime)
							{
								std::this_thread::yield();
							}

							disconnect(false);
						});
						ackTimeOut.detach();
						break;
					}
					if (type == MessageType::FINACK)
					{
						verboseInfo("raw received FINACK packet");
						disconnect(false);
						break;
					}

					unsigned short length = ntohs(*reinterpret_cast<unsigned short*>(packet + 1));
					if (length == 0 || recv(socket_, packet + meta, length, 0) == length)
					{
						processMutex.lock();
						idxQueuedMessagesEnd_ = (idxQueuedMessagesEnd_ + 1) % kMaxNumberOfMessages;
						verboseInfo("raw received packet type:" << (int)*reinterpret_cast<unsigned char*>(packet) << " length:" << length);
						processMutex.unlock();
						success = true;
					}
				}

				if( !success )
				{
					disconnect(true);
					setLastError("Error on receive");
				}
			}
			std::this_thread::yield();
		}
	}

	bool Connection::listenTo(const char * port, ConnectionListener &outListener, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets
 
		if (!isInit_)
			init();

		struct addrinfo hints, *results;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		int getAddrResult = getaddrinfo(NULL, port, &hints, &results);
		getAddrResult;
		SOCKET sockListen;
		sockListen = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
		if (sockListen == INVALID_SOCKET)
		{
			// Clean up
			setLastError("Error creating listen socket");
			freeaddrinfo(results);
			return false;
		}

		int reuseAddress = 1;
		if ( setsockopt(sockListen, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddress, sizeof reuseAddress) == SOCKET_ERROR)
		{
			setLastError("Error setting SO_REUSEADDR");
			printSockError("");
		}
		int ipv6only = 0;
		if( setsockopt(sockListen, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof ipv6only) == SOCKET_ERROR)
		{
			setLastError("Error setting IPV6_V6ONLY");
			printSockError("");
		}
		
		if (bind(sockListen, results->ai_addr, results->ai_addrlen) == SOCKET_ERROR)
		{
			// Clean up
			setLastError("Error binding listen socket");
			closesocket(sockListen);
			freeaddrinfo(results);
			return false;
		}
		
		if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR)
		{
			// Clean up
			setLastError("Error listening listen socket");
			closesocket(sockListen);
			freeaddrinfo(results);
			return false;
		}

		outListener.socket_ = sockListen;
		outListener.isListening_ = true;
		*reinterpret_cast<sockaddr*>(outListener.address_) = *results->ai_addr;
		
		freeaddrinfo(results);
		return true;
	}

	bool Connection::connectTo(const char * host, const char * port, Connection &outConnection, unsigned int timeout)
	{
		timeout; // TODO: nonblocking sockets

		if (!isInit_)
			init();

		struct addrinfo hints, *results;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		int getAddrResult = getaddrinfo(host, port, &hints, &results);
		getAddrResult;
		SOCKET sock = 0;

		bool success = false;

		for (addrinfo* addressToTest = results; addressToTest != nullptr; addressToTest = addressToTest->ai_next )
		{
			sock = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
			if (sock == INVALID_SOCKET)
			{
				setLastError("Error creating connecting socket");
				printSockError("");
				closesocket(sock);
				continue;
			}

			if (connect(sock, results->ai_addr, results->ai_addrlen) == SOCKET_ERROR)
			{
				setLastError("Error connecting to socket");
				printSockError("");
				closesocket(sock);
				continue;
			}

			success = true;
			break;
		}

		if (success)
		{
			outConnection.isConnected_ = true;
			outConnection.socket_ = sock;
			outConnection.isHosting_ = false;
			outConnection.run();
		}

		freeaddrinfo(results);

		return success;
	}

	void Connection::disconnect(bool waitForAck)
	{
		if (isConnected_)
		{
			if (waitForAck)
			{
				waitingForAck_ = true;
				sendPayload(MessageType::FIN);
			}
			else
			{
				shutdown(socket_, SHUT_RDWR);
				closesocket(socket_);
			}
			isConnected_ = false;
		}
		if (waitingForAck_ && !waitForAck)
		{
			shutdown(socket_, SHUT_RDWR);
			closesocket(socket_);
			waitingForAck_ = false;
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
			setLastError("Error on send payload");
			return false;
		}

		verboseInfo("raw sent packet type:" << type << " length:" << length);

		return true;
	}

	bool Connection::sendMessage(std::string message)
	{
		return sendPayload(MessageType::SEND_MESSAGE, message.c_str(), message.length() + 1);
	}

	bool Connection::sendWinner(int result)
	{
		char winner = (char)result;
		return sendPayload(MessageType::WINNER_RESULT, &winner, sizeof winner);
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

				if (message != nullptr && length != 0 && type == MessageType::SEND_MESSAGE)
				{
					outResponse = std::string(message);
					return true;
				}
				else
				{
					verboseInfo("received packet discarded type:" << type << " length:" << length);
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
                        
			memcpy(currentMessage_, buffer, outLength + 3);

			verboseInfo("received packet processed type:" << outType << " length:" << outLength);
			
			idxQueuedMessagesStart_ = (idxQueuedMessagesStart_ + 1) % kMaxNumberOfMessages;
			result = currentMessage_ + 3;
		}
		processMutex.unlock();
		return result;
	}

	bool Connection::isConnected() const
	{
		return isConnected_ || waitingForAck_;
	}

	bool Connection::isHosting() const
	{
		return isHosting_;
	}

	void ConnectionListener::end()
	{
		if (isListening_)
		{
			shutdown(socket_, SHUT_RDWR);
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

		struct sockaddr_storage incAddr;
		socklen_t incAddrSize = sizeof incAddr;


		SOCKET sockIncomingConnection = accept(socket_, (sockaddr*)&incAddr, &incAddrSize);
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
			return false;
		}
	}

}
