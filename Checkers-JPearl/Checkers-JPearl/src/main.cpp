#include <iostream>
#include <string>
#include <stdexcept>

#include "game.h"
#include "player.h"

int main(int argc, char ** argv)
{
	checkers::Game game = checkers::Game();
	game.initialize();
	int winner = game.getPlayer( game.run() )->getControllingSide();
	game.release();

	return winner;
}