#pragma once

#include <string>
#include <mutex>

#ifdef DEBUG
	#include <sstream>
	#include <iostream>
	//#define _X86_
	//#include <debugapi.h>
#define printSockError( message ) std::ostringstream os__ = std::ostringstream();char error[256];int code = Connection::getLastError(error, 256);os__ << message << "  " << code << " -- " << error << std::endl; std::cout << os__.str() << std::endl;//OutputDebugString(os__.str().c_str());
#else
	#define printSockError(message)
#endif

namespace checkers
{
	enum MessageType : unsigned char
	{
		SEND_MESSAGE = 0,
		REQUEST_INPUT = 1
	};
	class ConnectionListener;
	class Connection
	{
		static const int kMaxMessageSize = 1280;
		static const int kMaxNumberOfMessages = 5;
		static bool isInit_;

		unsigned int socket_;
		bool isHosting_;
		bool isConnected_;

		// Circular buffer of messages
		unsigned char idxQueuedMessagesStart_, idxQueuedMessagesEnd_;
		char queuedMessages_[kMaxMessageSize * kMaxNumberOfMessages];
		char currentMessage_[kMaxMessageSize];

		std::mutex sendMutex, processMutex;

		bool sendPayload(MessageType type, const char * data = nullptr, unsigned int length = 0);

		// Starts running this connection on a new thread and keeping track of new messages
		void run();
		void runLoop();
	public:
		static void init();
		static void shutdown();
		static int getLastError(char * buffer = nullptr, int bufferLength = 0);


		Connection();
		
		// Listens on the port given for any incoming connection and will write it to the given outConnection parameter. Returns whether a connection was found before timeout (in milliseconds)
		static bool listenTo(unsigned short port, ConnectionListener &outListener, unsigned int timeout = 1000);

		static bool connectTo(std::string ip, unsigned short port, Connection &outConnection, unsigned int timeout = 1000);

		void disconnect();

		// Sends a message to the other end. Returns whether it was successful
		bool sendMessage(std::string message);

		// Requests a message from the other end. Returns whether it was successful
		bool requestInput(std::string &outResponse);

		// Returns whether the connection has a message waiting and prepare it
		bool hasMessageWaiting();

		// If a message is waiting return it. Pointer is to the start of the payload. Payload is invalidated on next call to processMessage(). Meta information is returned through the parameters. If no message is waiting will return nullptr
		const char * processMessage(MessageType &outType, unsigned int &outLength);

		bool isConnected() const;
		bool isHosting() const;

		friend class ConnectionListener;
	};
	class ConnectionListener
	{
		unsigned int socket_;
		bool isListening_;
	public:
		void end();

		bool isListening() const;
		bool acceptConnection(Connection &outConnection, unsigned int timeout = 1000);
	
		friend class Connection;
	};
}
