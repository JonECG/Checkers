#pragma once


#include "thread_pool.h"

namespace checkers
{
	class EliminationTournament
	{
		unsigned int *brackets_;
		unsigned int *bracketsBackBuffer_;
		unsigned int *lossBracketIndices_;
		unsigned int *lossBracketIndicesBackBuffer_;

		unsigned int numTeams_;
		unsigned char numLossesBeforeElimination_;
	public:
		void initialize(unsigned int numTeams, unsigned char numLossesBeforeElimination);
		void release();

		// Runs the tournament. Provide the teams and a function that takes in two of the teams and returns the winner between them
		template <class T> void run(T *teams, std::function<T&(T&, T&)> matchFunction);

		// If the tournament has been ran, returns the list of teams in the reverse order they were eliminated
		// This means that the first X teams represent the top X teams where X is the number of losses per elimination
		// The teams are represented by an index into the array that was passed to run()
		const unsigned int * getBrackets() const;

		// Returns the number of places this tournament was able to determine
		int getNumResolvedPlaces() const;

		// Returns the teams that are in the given place. Place is 1-based (pass in 1 to get 1st place)
		const unsigned int* getTeamsAtPlace(int place, int &outCount) const;

		int getMaxNumMatches() const;

		// Inserts the current brackets into the stream
		friend std::ostream& operator<< (std::ostream& stream, const EliminationTournament& board);
	};


	template<class T>
	inline void EliminationTournament::run(T * teams, std::function<T&(T&, T&)> matchFunction)
	{
		ThreadPool pool;
		pool.initialize();

		// Tournament plays until there is only team not eliminated
		while (lossBracketIndices_[numLossesBeforeElimination_] != 1)
		{
			// Play normal matches from the bottom up as long as there are two 
			for (unsigned char bracket = 0; bracket < numLossesBeforeElimination_; bracket++)
			{
				unsigned char bracketIdx = numLossesBeforeElimination_ - bracket - 1;
				unsigned int start = lossBracketIndices_[bracketIdx];
				unsigned int end = lossBracketIndices_[bracketIdx + 1];
				unsigned int numInBracket = end - start;
				unsigned int matchStart = (numInBracket % 2) ? start + 1 : start; // If one is to be left alone, it will get a bye by being in the first index
				unsigned int numMatches = numInBracket / 2; // Note the integer division -- it allows the number of matches to be correctly calculated even if there is a bye

				unsigned int loseStart = matchStart + numMatches;

				unsigned int currentWinnerIndex = start;
				unsigned int currentLoserIndex = loseStart;

				if (start != matchStart)
					bracketsBackBuffer_[loseStart - 1] = brackets_[start]; // Team that gets a bye is put at the bottom of the bracket so it is forced to play next round

																		   // Only play games when the corresponding winner/X-time-loser bracket can pair off into pairs.
				for (unsigned int matchNumber = 0; matchNumber < numMatches; matchNumber++)
				{
					unsigned int teamAIdx = brackets_[matchStart + matchNumber * 2];
					unsigned int teamBIdx = brackets_[matchStart + matchNumber * 2 + 1];

					// Spin off a new job simulating the game. Writes winner and loser indices into bracketsBackbuffer at indices currentWinnerIndex and currentLoserIndex respectively
					pool.addJob([this, currentWinnerIndex, currentLoserIndex, teamAIdx, teamBIdx, teams, matchFunction] {
						T& result = matchFunction(teams[teamAIdx], teams[teamBIdx]);
						unsigned int winningIdx = (unsigned int)(&result - teams);
						unsigned int losingIdx = (winningIdx == teamAIdx) ? teamBIdx : teamAIdx;

						bracketsBackBuffer_[currentWinnerIndex] = winningIdx;
						bracketsBackBuffer_[currentLoserIndex] = losingIdx;
					});

					currentWinnerIndex++;
					currentLoserIndex++;
				}

				// The loser + 1 bracket just got a bit longer with the new games
				lossBracketIndicesBackBuffer_[bracketIdx + 1] = (start != loseStart) ? loseStart : end;
			}

			// Cross-Bracket matches can be played against brackets when there is only 1 team per the two highest brackets
			unsigned char teamABracket = numLossesBeforeElimination_;
			for (unsigned char bracket = 0; bracket < numLossesBeforeElimination_; bracket++)
			{
				unsigned int count = lossBracketIndices_[bracket + 1] - lossBracketIndices_[bracket];
				if (count > 1)
				{
					// We can't have a Cross-Bracket match if there are normal matches to be played
					break;
				}
				if (count == 1)
				{
					if (teamABracket == numLossesBeforeElimination_)
					{
						// This is the first team
						teamABracket = bracket;
					}
					else
					{

						// This is the second team -- have the Cross-Bracket match
						unsigned int teamAIdx = brackets_[lossBracketIndices_[teamABracket]];
						unsigned int teamBIdx = brackets_[lossBracketIndices_[bracket]];

						// Spins off the cross-bracket match
						pool.addJob([this, teamABracket, bracket, teamAIdx, teamBIdx, teams, matchFunction] {
							T& result = matchFunction(teams[teamAIdx], teams[teamBIdx]);
							unsigned int winningIdx = (unsigned int)(&result - teams);
							unsigned int losingIdx = (winningIdx == teamAIdx) ? teamBIdx : teamAIdx;

							bracketsBackBuffer_[lossBracketIndices_[teamABracket]] = winningIdx;
							bracketsBackBuffer_[lossBracketIndices_[bracket]] = losingIdx;

							// Have to update the lossBracketIndices specially with a Cross-Bracket match
							if (teamAIdx == winningIdx)
							{
								lossBracketIndicesBackBuffer_[bracket + 1]--;
							}
							else
							{
								for (unsigned char bracketToMove = teamABracket + 1; bracketToMove <= bracket; bracketToMove++)
								{
									lossBracketIndicesBackBuffer_[bracketToMove]--;
								}
							}
						});

						break; // Can't have more than one cross bracket match at a time
					}
				}
			}

			// Wait for the normal round to be complete
			pool.join();

			// Buffer swap
			for (unsigned int i = 0; i < lossBracketIndices_[numLossesBeforeElimination_]; i++)
			{
				brackets_[i] = bracketsBackBuffer_[i];
			}
			unsigned int *swap = lossBracketIndices_;
			lossBracketIndices_ = lossBracketIndicesBackBuffer_;
			lossBracketIndicesBackBuffer_ = swap;
		}

		pool.release();
	}
}
