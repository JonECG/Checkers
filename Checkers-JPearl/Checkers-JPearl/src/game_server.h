#pragma once
#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <thread>
#include <mutex>

#include "connection.h"
#include "game.h"

namespace checkers
{
	class Connection;
	class GameServer
	{
		static const int kMaxConnections = 20;

		bool isRunning_;
		
		std::thread runningThread_;
		std::mutex serverMutex_;

		int currentGameIndex_;
		Game currentGames_[kMaxConnections];

		ConnectionListener listener;

		int currentConnectionIndex_;
		std::thread instances_[kMaxConnections];
		Connection currentConnections_[kMaxConnections];
		Connection *connectionWaitingForOnlineGame_;

		void run();
		void initConnection(Connection &connection);
		void addOnlinePlayer(Connection &playerToAdd);
		int runGame(Game &game);
		void startAiGame(Connection &player, int aiDifficuluty);
		void startOnlineGame(Connection &playerOne, Connection &playerTwo);


	public:
		GameServer();

		void initialize();
		void release();

		// Starts the server, returns whether successful
		bool start(const char * port);
		void stop();

		bool isRunning();
	};
}

#endif // GAME_SERVER_H
