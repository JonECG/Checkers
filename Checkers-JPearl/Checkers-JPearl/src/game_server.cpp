#include "game_server.h"

#include <iostream>

#include "ai_player.h"
#include "network_player.h"

namespace checkers
{
	
	GameServer::GameServer()
	{
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
			if (currentConnectionIndex_ < kMaxConnections && listener.isListening())
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
					printSockError("Server error on accepting");
				}
			}
			std::this_thread::yield();
		}

		// Shutdown

		if (listener.isListening())
			listener.end();

		for (int i = 0; i < currentConnectionIndex_; i++)
		{
			if(currentConnections_[i].isConnected())
				currentConnections_[i].disconnect(false);

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

							std::this_thread::yield();
						}

						startAiGame(connection, levelResult);

						break;
					}
				}
			}

			if (!receivedValidInput)
				connection.sendMessage("Unrecognized input\n");

			std::this_thread::yield();
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
			Connection *connectionWaiting = connectionWaitingForOnlineGame_;
			connectionWaitingForOnlineGame_ = nullptr;
			serverMutex_.unlock();
			startOnlineGame(*connectionWaiting, playerToAdd);
		}
	}

	int GameServer::runGame(Game & game)
	{
		// The instance now belongs to the connection that started the game.
		// Which is the connection that requested the AI game or the 2nd connection into an online game
		game.initialize();
		int result = game.run();
		game.release();
		return result;
	}

	void GameServer::startAiGame(Connection & player, int aiDifficuluty)
	{
		serverMutex_.lock();

		int gameIndex = currentGameIndex_++;
		Game *game = currentGames_ + gameIndex;
		game->registerPlayer(new NetworkPlayer( &player), PieceSide::O);
		game->registerPlayer(new AiPlayer(aiDifficuluty), PieceSide::X);

		serverMutex_.unlock();

		int winner = runGame(currentGames_[gameIndex]);
		player.sendWinner(winner);
		player.disconnect(true); // Disconnect players on game completion
	}

	void GameServer::startOnlineGame(Connection & playerOne, Connection & playerTwo)
	{
		serverMutex_.lock();

		int gameIndex = currentGameIndex_++;
		Game *game = currentGames_ + gameIndex;
		game->registerPlayer(new NetworkPlayer(&playerOne), PieceSide::O);
		playerOne.sendMessage("\n\nYou are playing as O's\n\n");
		game->registerPlayer(new NetworkPlayer(&playerTwo), PieceSide::X);
		playerTwo.sendMessage("\n\nYou are playing as X's\n\n");

		serverMutex_.unlock();

		int winner = runGame(currentGames_[gameIndex]);
		playerOne.sendWinner(winner);
		playerTwo.sendWinner(winner);
		playerOne.disconnect(true); playerTwo.disconnect(true); // Disconnect players on game completion
	}

	bool GameServer::start(const char * port)
	{
		bool result = false;
		serverMutex_.lock();
		if (!isRunning_)
		{
			listener = ConnectionListener();
			if (!Connection::listenTo(port, listener, 1000))
			{
				printSockError("Server error on creating listener");
			}
			else
			{
				result = true;
				isRunning_ = true;
				currentConnectionIndex_ = 0;
				currentGameIndex_ = 0;
				runningThread_ = std::thread([this] { run(); });
			}
		}
		serverMutex_.unlock();
		return result;
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
