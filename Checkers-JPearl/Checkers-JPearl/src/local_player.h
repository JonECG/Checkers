#pragma once
#ifndef LOCAL_PLAYER_H
#define LOCAL_PLAYER_H

#include "player.h"
namespace checkers
{
	class LocalPlayer : public Player
	{
	public:
		const char * getDescriptor() const override;
		Move requestMove() override;
		void sendMessage(const char * message) const override;
	};
}

#endif // LOCAL_PLAYER_H
