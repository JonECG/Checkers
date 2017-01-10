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

			std::cout << "Enter a move as {start} {destination1} {destinationX...}  Eg: c3 d4 or e3 c5 e7" << std::endl;

			Move moves[Game::kMoveArraySize];
			int startIndex = 0;
			int count = game_->findAllMoves(getControllingSide(), moves, Game::kMoveArraySize, startIndex);

			std::cout << "Available Moves: " << std::endl;
			for (int i = 0; i < count; i++)
			{
				std::cout << "\t" << (i+1) << ") " << moves[i+startIndex] << std::endl;
			}

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
}
