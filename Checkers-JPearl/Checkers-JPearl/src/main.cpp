#include <iostream>
#include <string>
#include <stdexcept>

#include "game.h"
#include "player.h"
#include "local_player.h"
#include "ai_player.h"
#include "game_server.h"

int getAiLevel(const char * message)
{
	bool validInput = false;
	int result = 0;

	while (!validInput)
	{
		std::cout << message << std::endl << "Enter 0-9 > ";

		std::string choice;
		std::getline(std::cin, choice);

		if (choice.length() == 1 && choice[0] >= '0' && choice[0] <= '9' )
		{
			validInput = true;
			result = choice[0] - '0';
		}
	}

	return result;
}

int main(int argc, char ** argv)
{
	int winner = -1;
	
	bool repeat = true;

	checkers::GameServer gameServer(8989);

	gameServer.initialize();

	while (repeat)
	{
		std::cout <<
			"Welcome to Jonathan Pearl's implementation of American Checkers. Please select what you would like to do." << std::endl <<
			"1) Play a game locally against another person" << std::endl <<
			"2) Play a game against AI" << std::endl <<
			"3) Watch a game played between AIs" << std::endl <<
			( gameServer.isRunning() ? "4) Stop Network Game Server" : "4) Start Network Game Server" ) << std::endl <<
			"5) Quit" << std::endl <<
			"Your Choice > ";

		std::string choice;
		std::getline(std::cin, choice);

		if (choice.length() > 0)
		{

			switch (choice[0])
			{
			case '1':
			{
				checkers::Game game = checkers::Game();
				game.registerPlayer(new checkers::LocalPlayer(&game), checkers::PieceSide::O);
				game.registerPlayer(new checkers::LocalPlayer(&game), checkers::PieceSide::X);
				game.initialize();
				winner = game.getPlayer(game.run())->getControllingSide();
				game.release();
				break;
			}
			case '2':
			{
				checkers::Game game = checkers::Game();
				game.registerPlayer(new checkers::LocalPlayer(&game), checkers::PieceSide::O);
				game.registerPlayer(new checkers::AiPlayer(&game, getAiLevel("Enter the difficulty level for the AI")), checkers::PieceSide::X);
				game.initialize();
				winner = game.getPlayer(game.run())->getControllingSide();
				game.release();
				break;
			}
			case '3':
			{
				checkers::Game game = checkers::Game();
				game.registerPlayer(new checkers::AiPlayer(&game, getAiLevel("Enter the difficulty level for the AI playing 'O's")), checkers::PieceSide::O);
				game.registerPlayer(new checkers::AiPlayer(&game, getAiLevel("Enter the difficulty level for the AI playing 'X's")), checkers::PieceSide::X);
				game.initialize();
				winner = game.getPlayer(game.run())->getControllingSide();
				game.release();
				break;
			}
			case '4':
				if (gameServer.isRunning())
					gameServer.stop();
				else
					gameServer.start();
				break;
			case '5':
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