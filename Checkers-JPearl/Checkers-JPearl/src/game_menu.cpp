#include "game_menu.h"

#include <iostream>
#include <string>
#include <stdexcept>

#include "game.h"
#include "player.h"
#include "local_player.h"
#include "ai_player.h"
#include "game_server.h"

#include "dummy_client.h"

#include "ai_genetic_algorithm.h"

namespace checkers
{
	int GameMenu::promptAiLevel(const char * message) const
	{
		bool validInput = false;
		int result = 0;

		while (!validInput)
		{
			std::cout << message << std::endl << "Enter 0-9 > ";

			std::string choice;
			std::getline(std::cin, choice);

			if (choice.length() == 1 && choice[0] >= '0' && choice[0] <= '9')
			{
				validInput = true;
				result = choice[0] - '0';
			}
		}

		return result;
	}

	int GameMenu::playGame(Player *playerOs, Player *playerXs) const
	{
		checkers::Game game = checkers::Game(true);
		game.registerPlayer(playerOs, PieceSide::O);
		game.registerPlayer(playerXs, PieceSide::X);
		game.initialize();
		game.startGame();
		while (game.playTurn()){}
		int result = game.getWinner();
		game.release();
		return result;
	}

	int GameMenu::show() const
	{
		int winner = -1;

		bool repeat = true;

		checkers::GameServer gameServer;
		gameServer.initialize();

		while (repeat)
		{
			std::cout <<
				"Welcome to Jonathan Pearl's implementation of American Checkers. Please select what you would like to do." << std::endl <<
				"1) Play a game locally against another person" << std::endl <<
				"2) Play a game against AI" << std::endl <<
				"3) Watch a game played between AIs" << std::endl <<
				(gameServer.isRunning() ? "4) Stop Network Game Server" : "4) Start Network Game Server") << std::endl <<
				"5) Start as Client" << std::endl <<
				"6) Run AI Genetic Algorithm to train weights" << std::endl <<
				"7) Quit" << std::endl <<
				"Your Choice > ";

			std::string choice;
			std::getline(std::cin, choice);

			if (choice.length() > 0)
			{
				switch (choice[0])
				{
				case '1':
					winner = playGame(new LocalPlayer(), new LocalPlayer());
					break;
				case '2':
					winner = playGame(new LocalPlayer(), new AiPlayer(promptAiLevel("Enter the difficulty level for the AI")));
					break;
				case '3':
					winner = playGame(new checkers::AiPlayer(promptAiLevel("Enter the difficulty level for the AI playing 'O's")),
						new AiPlayer(promptAiLevel("Enter the difficulty level for the AI playing 'X's")));
					break;
				case '4':
					if (gameServer.isRunning())
					{
						gameServer.stop();
					}
					else
					{
						std::cout << "Enter Port (default " << checkers::DummyClient::kDefaultPort << ") > ";
						std::string port = std::string();
						std::getline(std::cin, port);
						if (port.empty())
							port = checkers::DummyClient::kDefaultPort;

						if (!gameServer.start(port.c_str()))
							std::cout << "Couldn't start server on port " << port << std::endl;
					}
					break;
				case '5':
					DummyClient dummy;
					winner = dummy.run();
					break;
				case '6':
				{
					AiGeneticAlgorithm trainer = AiGeneticAlgorithm();
					trainer.initialize(4, 0.1, 5, "./fittestByGeneration.csv");
					trainer.randomize(10);
					for (int i = 0; i < 100; i++)
					{
						trainer.processGeneration();
						std::cout << trainer.getFittest();

						// Write fittest to csv
					}
					trainer.release();
					break;
				}
				case '7':
					repeat = false;
					break;
				default:
					std::cout << "Unrecognized input" << std::endl;
					break;
				}

			}
		}

		gameServer.release();

		return winner;
	}
}
