#pragma once
#ifndef DUMMY_CLIENT_H
#define DUMMY_CLIENT_H

namespace checkers
{
	class DummyClient
	{
	public:
		static const char * kDefaultPort;

		int run();
	};
}

#endif // DUMMY_CLIENT_H
