#include "player.h"

#include <iostream>
#include <string>

#include "move.h"

namespace checkers
{
	Move Player::requestMove() const
	{
		Move result;

		bool moveSuccessfullyMade = false;

		while (!moveSuccessfullyMade)
		{

			std::cout << "Enter a move as {start} {destination1} {destinationX...}  Eg: c3 d4" << std::endl;
			std::cout << "player '" << getSymbol() << "'> ";

			std::string input;
			std::getline(std::cin, input);

			try
			{
				result = checkers::Move::parseFromString(input.c_str());
				moveSuccessfullyMade = true;
			}
			catch (std::exception& e)
			{
				std::cout << "Invalid formatting: " << e.what() << std::endl;
			}
		}
		return result;
	}
	char Player::getSymbol() const
	{
		return (controllingSide_ == PieceSide::O) ? 'o' : 'x';
	}

	PieceSide Player::getControllingSide() const
	{
		return controllingSide_;
	}

	void Player::setControllingSide(PieceSide side)
	{
		controllingSide_ = side;
	}

}