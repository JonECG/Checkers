#include "game_server.h"

#include "ai_player.h"
#include "network_player.h"

namespace checkers
{
	

	GameServer::GameServer(int port)
	{
		port_ = port;
		isRunning_ = false;
	}

	void GameServer::initialize()
	{
	}

	void GameServer::release()
	{
		if (isRunning_)
		{
			stop();
		}
	}

	void GameServer::run()
	{
		while (isRunning_)
		{
			if (currentConnectionIndex_ < kMaxConnections)
			{
				Connection &potential = currentConnections_[currentConnectionIndex_];
				if (Connection::listen(port_, potential, 1000))
				{
					currentConnectionIndex_++;
					std::thread connectionInit = std::thread([this, &potential] {initConnection(potential); });
					connectionInit.detach();
				}
			}
		}

		// Shutdown
		for (int i = 0; i < currentConnectionIndex_; i++)
		{
			if(currentConnections_[i].isConnected())
				currentConnections_[i].disconnect();
		}
	}

	void GameServer::initConnection(Connection & connection)
	{
		bool receivedValidInput = false;

		while (!receivedValidInput)
		{
			connection.sendMessage(
				"Welcome to the server. What would you like to do?\n"
				"1) Play against someone else\n"
				"2) Play against an AI\n"
				"Your choice > "
			);

			std::string response = "1";// connection.requestInput();

			if (response.length() == 1)
			{
				switch (response[0])
				{
					case '1': // Play against someone else
						receivedValidInput = true;

						addOnlinePlayer(connection);

						break;
					case '2': // Play against an AI
					{
						receivedValidInput = true;

						bool receivedValidAILevel = false;
						int levelResult = 0;

						while (!receivedValidAILevel)
						{
							connection.sendMessage("Enter the difficulty level for the AI\nEnter 0-9 > ");

							std::string levelResponse = connection.requestInput();
							
							if (levelResponse.length() == 1 && levelResponse[0] >= '0' && levelResponse[0] <= '9')
							{
								receivedValidAILevel = true;
								levelResult = levelResponse[0] - '0';
							}
						}

						startAiGame(connection, levelResult);

						break;
					}
				}
			}

			if (!receivedValidInput)
				connection.sendMessage("Unrecognized input\n");
		}
	}

	void GameServer::addOnlinePlayer(Connection & playerToAdd)
	{
		serverMutex_.lock();

		if (connectionWaitingForOnlineGame_ == nullptr || !connectionWaitingForOnlineGame_->isConnected())
		{
			playerToAdd.sendMessage("Waiting for another player to join.\n");
			connectionWaitingForOnlineGame_ = &playerToAdd;
			serverMutex_.unlock();
		}
		else
		{
			playerToAdd.sendMessage("Found another player! Starting game.\n");
			connectionWaitingForOnlineGame_->sendMessage("Found another player! Starting game.\n");
			serverMutex_.unlock();
			startOnlineGame(*connectionWaitingForOnlineGame_, playerToAdd);
		}
	}

	void GameServer::startGame(Game & game)
	{
		serverMutex_.lock();
		std::thread gameThread = std::thread([this, &game] { game.initialize(); game.run(); game.release(); });
		gameThread.detach();
		serverMutex_.unlock();
	}

	void GameServer::startAiGame(Connection & player, int aiDifficuluty)
	{
		serverMutex_.lock();

		int gameIndex = currentGameIndex_++;
		Game *game = currentGames_ + gameIndex;
		game->registerPlayer(new NetworkPlayer(game, &player), PieceSide::O);
		game->registerPlayer(new AiPlayer(game, aiDifficuluty), PieceSide::X);

		serverMutex_.unlock();

		startGame(currentGames_[gameIndex]);
	}

	void GameServer::startOnlineGame(Connection & playerOne, Connection & playerTwo)
	{
		serverMutex_.lock();

		int gameIndex = currentGameIndex_++;
		Game *game = currentGames_ + gameIndex;
		game->registerPlayer(new NetworkPlayer(game, &playerOne), PieceSide::O);
		game->registerPlayer(new NetworkPlayer(game, &playerTwo), PieceSide::X);

		serverMutex_.unlock();

		startGame(currentGames_[gameIndex]);
	}

	void GameServer::start()
	{
		serverMutex_.lock();
		if (!isRunning_)
		{
			isRunning_ = true;
			runningThread_ = std::thread([this] { run(); });
		}
		serverMutex_.unlock();
	}

	void GameServer::stop()
	{
		serverMutex_.lock();
		if (isRunning_)
		{
			isRunning_ = false;
			serverMutex_.unlock();
			runningThread_.join();
		}
		else
		{
			serverMutex_.unlock();
		}
		serverMutex_.unlock();
	}

	bool GameServer::isRunning()
	{
		return isRunning_;
	}

}