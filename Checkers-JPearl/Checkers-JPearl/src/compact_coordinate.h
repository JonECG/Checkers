#pragma once
#ifndef COMPACT_COORDINATE_H
#define COMPACT_COORDINATE_H

namespace checkers
{
	struct CompactCoordinate
	{
		unsigned char row : 4;
		unsigned char column : 4;
	};
}

#endif // COMPACT_COORDINATE_H
