#include <iostream>
#include <string>
#include <stdexcept>

#include "checker_board.h"
#include "move.h"

int main(int argc, char ** argv)
{
	checkers::CheckerBoard board;
	board.initialize();

	bool looping = true;

	while (looping)
	{
		std::cout << board;

		std::cout << "Enter a move as {start} {destination1} {destinationX...}  Eg: c3 d4" << std::endl;
		std::cout << "player '*'> ";

		std::string input;
		std::getline(std::cin, input);

		try
		{
			checkers::Move move = checkers::Move::parseFromString(input.c_str());

			std::cout << "Parsed coords: ";
			for (int i = 0; i < move.getNumCoords(); i++)
			{
				checkers::CompactCoordinate coord = move.getCoordinate(i);
				std::cout << "< " << std::to_string(coord.column) << ", " << std::to_string(coord.row) << " >; ";
			}
			std::cout << std::endl;
		}
		catch (std::exception& e)
		{
			std::cout << "EXCEPTION: " << e.what() << std::endl;
		}

		std::cout << std::endl << std::endl;
	}

	board.release();

}