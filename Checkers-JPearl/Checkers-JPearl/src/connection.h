#pragma once
#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <mutex>

#ifdef DEBUG
	#include <iostream>
	#define printSockError( message ) { char error[256]; const char * customMessage = nullptr; int code = checkers::Connection::getLastError(error, 256, &customMessage);std::cout << message << "  " << ((customMessage == nullptr) ? "" : customMessage )<< code << " -- " << error << std::endl; }
#else
	#define printSockError(message) message;
#endif

namespace checkers
{
	enum MessageType : unsigned char
	{
		SEND_MESSAGE = 0,
		REQUEST_INPUT = 1,
		WINNER_RESULT = 2,
		FIN = 3,
		FINACK = 4
	};
	class ConnectionListener;
	class Connection
	{
		static const int kMaxMessageSize = 1280;
		static const int kMaxNumberOfMessages = 3;
		static int lastError_;
		static bool isInit_;
		static const char * connectionErrorMessage_;
		static const int kAckTimeoutMilliseconds = 1000; // Wait 1 second and assume ACK was received
		
		unsigned int socket_;
		bool isHosting_:1;
		bool isConnected_:1;
		bool waitingForAck_:1;

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
		static void cleanup();
		static int getLastError(char * buffer = nullptr, int bufferLength = 0, const char ** outCustomMessage = nullptr);

		Connection();
		
		// Listens on the port given for any incoming connection and will write it to the given outConnection parameter. Returns whether a connection was found before timeout (in milliseconds)
		static bool listenTo(const char * port, ConnectionListener &outListener, unsigned int timeout = 1000);

		static bool connectTo(const char * host, const char * port, Connection &outConnection, unsigned int timeout = 1000);

		void disconnect(bool waitForSendToComplete = true);

		// Sends a message to the other end. Returns whether it was successful
		bool sendMessage(std::string message);

		// Sends the winner index to the other end. Returns whether it was successful
		bool sendWinner(int result);

		// Requests a message from the other end. Returns whether it was successful
		bool requestInput(std::string &outResponse);

		// Yields current thread until this connection has a message or disconnects. Returns whether there is a message.
		bool waitUntilHasMessage() const;

		// Returns whether the connection has a message waiting and prepare it
		bool hasMessageWaiting() const;

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
		char address_[32];
	public:
		void end();

		bool isListening() const;
		bool acceptConnection(Connection &outConnection, unsigned int timeout = 1000);
	
		friend class Connection;
	};
}

#endif // CONNECTION_H
