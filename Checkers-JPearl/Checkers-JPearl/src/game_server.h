#pragma once

#include <thread>
#include <mutex>

#include "connection.h"
#include "game.h"

namespace checkers
{
	class Connection;
	class GameServer
	{
		static const int kMaxNumGames = 128;
		static const int kMaxConnections = 128;

		unsigned short port_;
		bool isRunning_;

		std::thread runningThread_;
		std::mutex serverMutex_;

		int currentGameIndex_;
		Game currentGames_[kMaxNumGames];

		int currentConnectionIndex_;
		Connection currentConnections_[kMaxConnections];

		Connection *connectionWaitingForOnlineGame_;

		void run();
		void initConnection(Connection &connection);
		void addOnlinePlayer(Connection &playerToAdd);
		void startGame(Game &game);
		void startAiGame(Connection &player, int aiDifficuluty);
		void startOnlineGame(Connection &playerOne, Connection &playerTwo);


	public:
		GameServer(unsigned short port);

		void initialize();
		void release();

		void start();
		void stop();

		bool isRunning();
	};
}
