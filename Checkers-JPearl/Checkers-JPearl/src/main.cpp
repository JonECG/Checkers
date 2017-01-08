#include <iostream>

#include "checker_board.h"

int main(int argc, char ** argv)
{
	checkers::CheckerBoard board;
	board.initialize();

	std::cout << board;

	std::cout << "Press enter to close";

	// Hang until enter
	std::getchar();

	board.release();

}