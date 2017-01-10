#pragma once

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
