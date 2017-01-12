#include "ai_genetic_algorithm.h"

#include "game.h"

namespace checkers
{
	void AiGeneticAlgorithm::resetScores()
	{
		for (unsigned char i = 0; i < populationSize_; i++)
		{
			scores_[i] = 0;
		}
	}
	void AiGeneticAlgorithm::calculateFitness()
	{
		int currentThreadIdx = 0;

		for (unsigned char brainAIdx = 0; brainAIdx < populationSize_; brainAIdx++)
		{
			for (unsigned char brainBIdx = 0; brainBIdx < populationSize_; brainBIdx++)
			{
				if (brainAIdx == brainBIdx)
					continue; // We can't have the same brain fight itself

				instances_[currentThreadIdx] = std::thread([this, currentThreadIdx, brainAIdx, brainBIdx] {
					Game game = Game();
					game.registerPlayer(new AiPlayer(aiRecurseLevels_, population_[brainAIdx].brain), PieceSide::O);
					game.registerPlayer(new AiPlayer(aiRecurseLevels_, population_[brainBIdx].brain), PieceSide::X);
					game.initialize();
					game.startGame();
					while (game.playTurn())
					{
						std::this_thread::yield();
					}
					int winner = game.getWinner();
					switch(winner)
					{
					case PieceSide::O:
						scores_[brainAIdx].fetch_add(2);
						break;
					case PieceSide::X:
						scores_[brainBIdx].fetch_add(2);
						break;
					default:
						scores_[brainAIdx].fetch_add(1);
						scores_[brainBIdx].fetch_add(1);
					}
					game.release();
				});

				currentThreadIdx++;
			}
		}

		// Wait for all of the results
		for (int i = 0; i < currentThreadIdx; i++)
		{
			instances_[i].join();
		}
	}
	void AiGeneticAlgorithm::reviewFitness()
	{
		AiPlayer::Brain *fittest = &population_[0].brain;
		int fittestScore = scores_[0];

		int currentBucketIdx = 0;

		for (unsigned char i = 0; i < populationSize_; i++)
		{
			if (scores_[i] > fittestScore)
			{
				fittestScore = scores_[i];
				fittest = &population_[i].brain;
			}

			for (int fill = 0; fill < scores_[i]; fill++)
			{
				buckets_[currentBucketIdx++] = i;
			}
		}

		fittest_ = *fittest;
	}
	void AiGeneticAlgorithm::mate(const AiPlayer::BrainView &parentA, const AiPlayer::BrainView &parentB, AiPlayer::BrainView &outOffspring) const
	{
		// We don't ever mutate the first weight -- we keep it at one in order to have it as a constant reference
		for (int weightIdx = 1; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
		{
			// Randomly choose which parent to take trait
			outOffspring.raw[weightIdx] = (std::rand() & 1) ? parentA.raw[weightIdx] : parentB.raw[weightIdx];
		}
	}
	void AiGeneticAlgorithm::produceOffspring()
	{
		AiPlayer::Brain *offSpring = new AiPlayer::Brain[populationSize_];

		// Most fit is guaranteed to be in new generation unchanged
		offSpring[0] = fittest_;

		// Create offspring
		for (int i = 1; i < populationSize_; i++)
		{
			AiPlayer::BrainView *parentA;
			unsigned char parentAIdx = buckets_[std::rand() % totalFitnessScore_]; // Weighted random
			parentA = population_ + parentAIdx;

			AiPlayer::BrainView *parentB;
			unsigned char parentBIdx = buckets_[std::rand() % totalFitnessScore_]; // Weighted random
			parentB = population_ + parentBIdx;

			mate(*parentA, *parentB, *reinterpret_cast<AiPlayer::BrainView*>(offSpring+i));
		}

		// Fill new generation
		for (int i = 0; i < populationSize_; i++)
		{
			population_[i].brain = offSpring[i];
		}

		delete[] offSpring;
	}
	void AiGeneticAlgorithm::mutate()
	{
		for (int brainIdx = 1; brainIdx < populationSize_; brainIdx++)
		{
			double randomStrength = (maxRandom_ * brainIdx) / populationSize_;

			// We don't ever mutate the first weight -- we keep it at one in order to have it as a constant reference
			for (int weightIdx = 1; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
			{
				double fraction = std::rand() / (double)RAND_MAX;
				fraction = randomStrength * (fraction * 2 - 1);
				population_[brainIdx].raw[weightIdx] += fraction;
			}
		}
	}
	void AiGeneticAlgorithm::initialize(unsigned char populationSize, double maxRandom, int aiRecurseLevels )
	{
		populationSize_ = populationSize;
		maxRandom_ = maxRandom;
		aiRecurseLevels_ = aiRecurseLevels;

		// Round Robin 1 point per win, but each pair plays two games so each side has a chance of going first as O
		numberOfInstances_ = populationSize_ * (populationSize_ - 1);
		totalFitnessScore_ = numberOfInstances_ * 2; // Two points awarded per game 

		// Allocate arrays
		population_ = reinterpret_cast<AiPlayer::BrainView*>(new AiPlayer::Brain[populationSize_]);
		instances_ = new std::thread[numberOfInstances_];
		scores_ = new std::atomic<unsigned char>[populationSize_];
		buckets_ = new unsigned char[totalFitnessScore_];

		// Seed random with current time
		std::srand((unsigned int)time(0));

		// Initialize population with current default brain
		for (int i = 0; i < populationSize_; i++)
		{
			population_[i].brain = AiPlayer::kDefaultBrain;
		}
		mutate();
		fittest_ = population_[0].brain;
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
		resetScores();
		calculateFitness();
		reviewFitness();
		produceOffspring();
		mutate();
	}

	AiPlayer::Brain AiGeneticAlgorithm::getFittest() const
	{
		return fittest_;
	}

}
