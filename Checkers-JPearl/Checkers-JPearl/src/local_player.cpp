#include "local_player.h"

#include <iostream>
#include <string>

#include "move.h"
#include "game.h"

namespace checkers
{
	LocalPlayer::LocalPlayer(Game * game)
	{
		game_ = game;
	}

	Move LocalPlayer::requestMove()
	{
		Move result;

		bool moveSuccessfullyMade = false;

		while (!moveSuccessfullyMade)
		{
			std::string input;
			std::getline(std::cin, input);

			try
			{
				result = checkers::Move::parseFromString(input.c_str());
				moveSuccessfullyMade = true;
			}
			catch (std::exception& e)
			{
				std::cout << "Invalid formatting: " << e.what() << std::endl << "Try again > ";
			}
		}
		return result;
	}
	void LocalPlayer::sendMessage(const char * message) const
	{
		message;
	}
}
