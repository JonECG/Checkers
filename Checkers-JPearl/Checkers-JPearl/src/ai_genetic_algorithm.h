#pragma once
#ifndef AI_GENETIC_ALGORITHM_H
#define AI_GENETIC_ALGORITHM_H

#include <thread>
#include <atomic>
#include <fstream>

#include "ai_player.h"

namespace checkers
{
	class AiGeneticAlgorithm
	{
		unsigned int populationSize_;

		int aiRecurseLevels_;
		double maxRandomPerGeneration_;
		double topPercent_;

		// The current generation of brains
		AiPlayer::BrainView *population_;
		// The collections of scores for each brain
		float *scores_;
		// An array to represent weighted random for fast indexing during offspring
		unsigned int *sortedIndices_;
		float *sortedCumulativeScores_;
		float totalFitnessScore_;

		std::ofstream recordFile_;
		AiPlayer::Brain fittest_;

		// Zeroes out scores from last calculateFitness()
		void resetScores();
		// Scores the fitness of every individual in the population
		void calculateFitness();
		// Sorts the given indices in respect to the values in the given array (least to greatest)
		void sortIndices(unsigned int *indices, const float *data, int count) const;
		// Reviews the fitness results from calculateFitness and populates the weighted buckets accordingly
		void reviewFitness();
		// Finds a parent at random weighted by scores
		unsigned int findParent() const;
		unsigned int findParent(unsigned int toExclude) const;
		// Mates the two parents and outputs the resulting offspring in outOffspring
		void mate(const AiPlayer::BrainView &parentA, const AiPlayer::BrainView &parentB, AiPlayer::BrainView &outOffspring) const;
		// Creates a new generation by splicing pairs based on the fitness of previous one and will always keep a copy of the best performing individual unchanged
		void produceOffspring();
		// Multiplicatively normalizes the brain values so that they are on the same average magnitude
		void normalize(AiPlayer::BrainView &brain, double magnitude = 1) const;
		// Mutates all members of the generation other than the fittest in varying degrees of randomness
		void mutate();
		// Writes fittest to end of file in csv
		void recordFittest();

	public:
		// Initializes the GA with the given population size (raise 2 to the given power) and the limit of randomness that can occur each generation. Can also provide the topmost percent of individuals to consider when choosing parents.
		// If given a path, will append to it in csv with the fittest brain's values creating a new file if it doesn't exist
		void initialize(unsigned char populationSizePow2, int aiRecurseLevels, double maxRandomPerGeneration = 0.125, double topPercent = 0.5, const char * outputCsvPath = nullptr);
		void release();

		// Randomizes all of the current generation in the range of -maxRandom to +maxRandom
		void randomize();

		// Processes a new generation. Get the fittest afterwards with getFittest(). 
		void processGeneration();

		// Returns a pointer to the current fittest as evaluated by the previous generation
		AiPlayer::Brain getFittest() const;
	};
}

#endif // AI_GENETIC_ALGORITHM_H
