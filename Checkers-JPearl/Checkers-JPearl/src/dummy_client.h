#pragma once
#ifndef DUMMY_CLIENT_H
#define DUMMY_CLIENT_H

namespace checkers
{
	class DummyClient
	{
	public:
		static const unsigned short kDefaultPort = 8989;

		void run();
	};
}

#endif // DUMMY_CLIENT_H
