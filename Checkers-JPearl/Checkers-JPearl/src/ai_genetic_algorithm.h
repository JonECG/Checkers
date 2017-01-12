#pragma once
#ifndef AI_GENETIC_ALGORITHM_H
#define AI_GENETIC_ALGORITHM_H

#include <thread>

#include "ai_player.h"

namespace checkers
{
	class AiGeneticAlgorithm
	{
		int populationSize_;
		int numberOfInstances_;
		int totalFitnessScore_;

		float maxRandom_;

		// The current generation of brains
		AiPlayer::BrainView *population_;
		// The simulations that set score between brains
		std::thread *instances_;
		// The collections of scores for each brain
		unsigned char *scores_;
		// An array to represent weighted random for fast indexing during offspring
		unsigned char *buckets_;

		AiPlayer::Brain *fittest_;

		// Scores the fitness of every individual in the population
		void calculateFitness();
		// Creates a new generation by splicing pairs based on the fitness of previous one and will always keep a copy of the best performing individual unchanged
		void produceOffspring();
		// Mutates all members of the generation other than the fittest in varying degrees of randomness
		void mutate();
	public:
		// Initializes the GA with the given population size and the limit of randomness that can occur each generation
		void initialize(int populationSize, float maxRandom);
		void release();
		void processGeneration();

		// Returns a pointer to the current fittest as evaluated by the previous generation
		AiPlayer::Brain * getFittest() const;
	};
}

#endif // AI_GENETIC_ALGORITHM_H
