#include "local_player.h"

#include <iostream>
#include <string>

#include "move.h"
#include "game.h"

namespace checkers
{
	const char * LocalPlayer::getDescriptor() const
	{
		return "";
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
				if (input.find("FORFEIT") != std::string::npos || input.find("forfeit") != std::string::npos)
					result.makeForfeit();
				else
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
