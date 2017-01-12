#include "game_menu.h"

#include <iostream>
#include <string>

int main(int argc, char ** argv)
{
	argc; argv;

	checkers::GameMenu menu;
	int result = menu.show();

	std::cout << "(" << result << ") Press enter to exit...";
	std::string dummy;
	std::getline(std::cin, dummy);

	return result;
}
