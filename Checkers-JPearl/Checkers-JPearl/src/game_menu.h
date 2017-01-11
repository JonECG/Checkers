#pragma once
#ifndef GAME_MENU_H
#define GAME_MENU_H

namespace checkers
{
	class Player;
	class GameMenu
	{
		// Given two players, plays a game of checkers and returns the winner
		int playGame(Player *playerOs, Player *playerXs) const;

		// Prompts user for an ai level from 0 to 9
		int promptAiLevel(const char * message) const;
	public:

		// Shows the main menu to the user
		int show() const;
	};
}

#endif // GAME_MENU_H
