#pragma once

#include <string>

namespace checkers
{
	enum MessageType : unsigned char
	{
		SEND_MESSAGE = 0,
		REQUEST_INPUT = 1
	};
	class Connection
	{
		static const int kMaxMessageSize = 256;
		static bool isInit_;

		unsigned int socket_;
		bool isHosting_;
		bool isConnected_;

		bool isMessageReady_;
		char currentMessage_[kMaxMessageSize];

		bool sendPayload(MessageType type, const char * data = nullptr, unsigned int length = 0) const;
	public:
		static void init();
		static void shutdown();

		void getLastError(char * buffer, int bufferLength);

		// Listens on the port given for any incoming connection and will write it to the given outConnection parameter. Returns whether a connection was found before timeout (in milliseconds)
		static bool listenTo(unsigned short port, Connection &outConnection, unsigned int timeout = 1000);

		static bool connectTo(std::string ip, unsigned short port, Connection &outConnection, unsigned int timeout = 1000);

		void disconnect();

		// Sends a message to the other end. Returns whether it was successful
		bool sendMessage(std::string message) const;

		// Requests a message from the other end. Returns whether it was successful
		bool requestInput(std::string &outResponse);

		// Returns whether the connection has a message waiting and prepare it
		bool hasMessageWaiting();

		// If a message is waiting return it. Pointer is to the start of the payload. Payload is invalidated on next call to hasMessageWaiting(). Meta information is returned through the parameters. If no message is waiting will return nullptr
		const char * processMessage(MessageType &outType, unsigned int &outLength) ;

		bool isConnected() const;
		bool isHosting() const;
	};
}