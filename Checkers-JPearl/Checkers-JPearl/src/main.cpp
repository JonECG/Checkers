#include <iostream>
#include <string>
#include <stdexcept>

#include "game.h"
#include "player.h"

int main(int argc, char ** argv)
{
	checkers::Game game = checkers::Game();
	game.initialize();

	/*
	char * initMoves[] = {
		"c3b4",
		"b6a5"
	};
	game.runMoves(initMoves, sizeof( initMoves ) / sizeof(char*));
	*/

	int winner = game.getPlayer( game.run() )->getControllingSide();
	game.release();

	std::cout << "Press enter to exit...";
	std::string dummy;
	std::getline(std::cin, dummy);


	return winner;
}