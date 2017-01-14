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
		unsigned char populationSize_;
		unsigned char numberOfInstances_;
		int totalFitnessScore_;

		int aiRecurseLevels_;
		double maxRandomPerGeneration_;

		// The current generation of brains
		AiPlayer::BrainView *population_;
		// The simulations that set score between brains
		std::thread *instances_;
		// The collections of scores for each brain
		std::atomic<unsigned char> *scores_;
		// An array to represent weighted random for fast indexing during offspring
		unsigned char *buckets_;

		std::ofstream recordFile_;
		AiPlayer::Brain fittest_;

		// Zeroes out scores from last calculateFitness()
		void resetScores();
		// Scores the fitness of every individual in the population
		void calculateFitness();
		// Reviews the fitness results from calculateFitness and populates the weighted buckets accordingly
		void reviewFitness();
		// Mates the two parents and outputs the resulting offspring in outOffspring
		void mate(const AiPlayer::BrainView &parentA, const AiPlayer::BrainView &parentB, AiPlayer::BrainView &outOffspring) const;
		// Creates a new generation by splicing pairs based on the fitness of previous one and will always keep a copy of the best performing individual unchanged
		void produceOffspring();
		// Multiplicatively normalizes the brain values so that they are on the same average magnitude
		void normalize(AiPlayer::BrainView &brain, double magnitude = 1);
		// Mutates all members of the generation other than the fittest in varying degrees of randomness
		void mutate();
		// Writes fittest to end of file in csv
		void recordFittest();

	public:
		// Initializes the GA with the given population size (raise 2 to the given power) and the limit of randomness that can occur each generation. If given a path, will append to it in csv with the fittest brain's values creating a new file if it doesn't exist
		void initialize(unsigned char populationSizePow2, double maxRandomPerGeneration, int aiRecurseLevels, const char * outputCsvPath = nullptr);
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
