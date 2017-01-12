#include "ai_genetic_algorithm.h"

namespace checkers
{
	void AiGeneticAlgorithm::calculateFitness()
	{
	}
	void AiGeneticAlgorithm::produceOffspring()
	{
	}
	void AiGeneticAlgorithm::mutate()
	{
	}
	void AiGeneticAlgorithm::initialize(int populationSize, float maxRandom)
	{
		populationSize_ = populationSize;
		maxRandom_ = maxRandom;

		// Round Robin 1 point per win
		numberOfInstances_ = populationSize_ * (populationSize_ - 1);
		totalFitnessScore_ = numberOfInstances_;

		// Allocate arrays
		population_ = reinterpret_cast<AiPlayer::BrainView*>(new AiPlayer::Brain[populationSize_]);
		instances_ = new std::thread[numberOfInstances_];
		scores_ = new unsigned char[populationSize_];
		buckets_ = new unsigned char[totalFitnessScore_];

		// Initialize population with current default brain
		for (int i = 0; i < populationSize_; i++)
		{
			population_[i].brain = AiPlayer::kDefaultBrain;
		}
		mutate();
		fittest_ = &population_[0].brain;
	}

	void AiGeneticAlgorithm::release()
	{
		// Deallocate arrays
		delete[] population_;
		delete[] instances_;
		delete[] scores_;
		delete[] buckets_;
	}

	void AiGeneticAlgorithm::processGeneration()
	{
	}

	AiPlayer::Brain * AiGeneticAlgorithm::getFittest() const
	{
		return fittest_;
	}

}
