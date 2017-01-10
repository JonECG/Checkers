#include "dummy_client.h"

#include <iostream>
#include <string>

int main(int argc, char ** argv)
{
	argc; argv;

	checkers::DummyClient client;
	client.run();

	std::cout << "Press enter to exit...";
	std::string dummy;
	std::getline(std::cin, dummy);
}
