#include "elimination_tournament.h"

#include <iostream>

namespace checkers
{
	void EliminationTournament::initialize(unsigned int numTeams, unsigned char numLossesBeforeElimination)
	{
		numTeams_ = numTeams;
		numLossesBeforeElimination_ = numLossesBeforeElimination;

		brackets_ = new unsigned int[numTeams];
		bracketsBackBuffer_ = new unsigned int[numTeams];
		lossBracketIndices_ = new unsigned int[numLossesBeforeElimination + 1];
		lossBracketIndicesBackBuffer_ = new unsigned int[numLossesBeforeElimination + 1];

		// Set up the brackets
		for (unsigned char i = 0; i < numTeams_; i++)
		{
			brackets_[i] = i;
		}

		// Store an index for the start of each bracket 0loss, 1loss, etc;
		lossBracketIndices_[0] = 0;
		lossBracketIndicesBackBuffer_[0] = 0;
		for (unsigned char i = 1; i <= numLossesBeforeElimination; i++)
		{
			lossBracketIndices_[i] = numTeams_;
		}
	}

	void EliminationTournament::release()
	{
		delete[] brackets_;
		delete[] bracketsBackBuffer_;
		delete[] lossBracketIndices_;
		delete[] lossBracketIndicesBackBuffer_;
	}

	const unsigned int * EliminationTournament::getBrackets() const
	{
		return brackets_;
	}

	int EliminationTournament::getNumResolvedPlaces() const
	{
		// Can place as many as the number of eliminations. The next group of size equal to the number of eliminations is the next rank. The rest are ranked last
		unsigned count = numLossesBeforeElimination_ + (numTeams_ > numLossesBeforeElimination_*2U ? 2 : 1); 
		if (count > numTeams_)
			count = numTeams_;
		return count;
	}

	const unsigned int * EliminationTournament::getTeamsAtPlace(int place, int & outCount) const
	{
		outCount = 0;

		if (place <= 0 || place > getNumResolvedPlaces())
			return nullptr; 

		if (place <= numLossesBeforeElimination_)
		{
			outCount = 1;
			return brackets_ + (place - 1);
		}
		if (place == numLossesBeforeElimination_+1)
		{
			int countPastIndividualRanks = numTeams_ - numLossesBeforeElimination_;
			outCount = (countPastIndividualRanks < numLossesBeforeElimination_) ? countPastIndividualRanks : numLossesBeforeElimination_;
			return brackets_ + numLossesBeforeElimination_;
		}

		outCount = numTeams_ - (numLossesBeforeElimination_*2);
		return brackets_ + (numLossesBeforeElimination_*2);
	}

	int EliminationTournament::getMaxNumMatches() const
	{
		return numTeams_ * numLossesBeforeElimination_ - 1;
	}

	std::ostream & operator<<(std::ostream & stream, const EliminationTournament & tourn)
	{
		for (int bracket = 0; bracket <= tourn.numLossesBeforeElimination_; bracket++)
		{
			stream << "(" << bracket << ") ";
			unsigned int start = tourn.lossBracketIndices_[bracket];
			unsigned int end = bracket == tourn.numLossesBeforeElimination_ ? tourn.numTeams_ : tourn.lossBracketIndices_[bracket + 1];
			unsigned int numInBracket = end - start;

			for (unsigned int i = 0; i < numInBracket; i++)
			{
				stream << (int)tourn.brackets_[start + i] << ((bracket != tourn.numLossesBeforeElimination_ && ((i % 2 == 0) ^ (numInBracket % 2 != 0))) ? "-" : " ");
			}
		}

		return stream;
	}

}
