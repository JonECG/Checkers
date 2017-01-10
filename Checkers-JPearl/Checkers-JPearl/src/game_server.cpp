#include "game_server.h"

#include <iostream>

#include "ai_player.h"
#include "network_player.h"

namespace checkers
{
	
	GameServer::GameServer(unsigned short port)
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
				// Start Listening if we aren't
				if (!listener.isListening())
				{
					if (!Connection::listenTo(port_, listener, 1000))
					{
#ifdef DEBUG
						char error[256];
						Connection::getLastError(error, 256);
						std::cout << "Server Error on creating listener: " << error << std::endl;
#endif // DEBUG
					}
				}
				else
				{
					// Try to accept a new connection
					Connection &potential = currentConnections_[currentConnectionIndex_];
					if (listener.acceptConnection(potential, 1000))
					{
						instances_[currentConnectionIndex_] = std::thread([this, &potential] {initConnection(potential);});
						currentConnectionIndex_++;
					}
					else
					{
#ifdef DEBUG
						char error[256];
						Connection::getLastError(error, 256);
						std::cout << "Server Error on accepting: " << error << std::endl;
#endif // DEBUG
					}
				}
			}
		}

		// Shutdown

		if (listener.isListening())
			listener.end();

		for (int i = 0; i < currentConnectionIndex_; i++)
		{
			if(currentConnections_[i].isConnected())
				currentConnections_[i].disconnect();

			instances_[i].join();
		}
		
	}

	void GameServer::initConnection(Connection & connection)
	{
		bool receivedValidInput = false;

		while (connection.isConnected() && !receivedValidInput)
		{
			if (!connection.sendMessage(
				"Welcome to the server. What would you like to do?\n"
				"1) Play against someone else\n"
				"2) Play against an AI\n"
				"Your choice > "
			))
				continue;

			std::string response = std::string();

			if (!connection.requestInput(response))
				continue;

			if ( response.length() == 1)
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

						while (connection.isConnected() && !receivedValidAILevel)
						{
							if (!connection.sendMessage("Enter the difficulty level for the AI\nEnter 0-9 > "))
								continue;

							std::string levelResponse = std::string();
							
							if (!connection.requestInput(levelResponse))
								continue;
							
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
		// The instance now belongs to the connection that started the game.
		// Which is the connection that requested the AI game or the 2nd connection into an online game
		game.initialize();
		game.run();
		game.release();
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
			listener.end();
			serverMutex_.unlock();
			runningThread_.join();
		}
		else
		{
			serverMutex_.unlock();
		}
	}

	bool GameServer::isRunning()
	{
		return isRunning_;
	}

}
