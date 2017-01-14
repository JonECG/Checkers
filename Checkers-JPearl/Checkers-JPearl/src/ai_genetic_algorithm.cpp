#include "ai_genetic_algorithm.h"

#include <fstream>

#include <mutex>
#include <iostream>

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
		// Poor man's thread pool
		unsigned char* freeThreadIndices = new unsigned char[numberOfInstances_];
		unsigned char freeThreadIndicesStart = 0;
		unsigned char freeThreadIndicesEnd = 0;
		std::atomic<int> numFinishedGames = 0;
		std::mutex poolLock;
		for (unsigned char i = 0; i < numberOfInstances_; i++)
		{
			freeThreadIndices[i] = i;
		}


		// Show a progress bar
		const int barWidth = 50;
		const char progressBarBackground = (unsigned char)176;
		const char progressBarForeground = (unsigned char)178;
		std::mutex progressLock;
		for (int i = 0; i < barWidth; i++)
			std::cout << progressBarBackground;
		std::cout << '\r' << std::flush;
		int currentProgressPoint = 0;


		// Set up the brackets
		unsigned char *brackets = new unsigned char[populationSize_];
		unsigned char *bracketsBackbuffer = new unsigned char[populationSize_];
		for (unsigned char i = 0; i < populationSize_; i++)
		{
			brackets[i] = i;
		}

		unsigned char roundSize = populationSize_;

		while (roundSize > 1)
		{
			// Divide the round size in half
			unsigned char roundHalfSize = roundSize >> 1;

			for (unsigned char roundStartIndex = 0; roundStartIndex < populationSize_; roundStartIndex += roundSize)
			{
				unsigned char currentWinnerIndex = 0;
				unsigned char currentLoserIndex = roundHalfSize;

				for (unsigned char currentMatch = 0; currentMatch < roundHalfSize; currentMatch++)
				{
					unsigned char brainAIdx = brackets[roundStartIndex + currentMatch * 2];
					unsigned char brainBIdx = brackets[roundStartIndex + currentMatch * 2 + 1];


					// Wait until a thread is available
					unsigned char availableThread = numberOfInstances_;
					while (availableThread == numberOfInstances_)
					{
						poolLock.lock();
						unsigned char firstClosedSpot = (freeThreadIndicesEnd + 1) % numberOfInstances_;
						if (freeThreadIndicesStart != firstClosedSpot)
						{
							availableThread = freeThreadIndices[freeThreadIndicesEnd];
							freeThreadIndicesEnd = firstClosedSpot;
						}
						poolLock.unlock();
						std::this_thread::yield();
					}

					// Make sure it's finished
					if (instances_[availableThread].joinable())
						instances_[availableThread].join();

					// Spin off a new thread simulating the game
					instances_[availableThread] = std::thread([this, &numFinishedGames, brainAIdx, brainBIdx, bracketsBackbuffer, currentWinnerIndex, currentLoserIndex, &progressLock, progressBarForeground,
																barWidth, &currentProgressPoint, availableThread, &poolLock, freeThreadIndices, &freeThreadIndicesStart, &freeThreadIndicesEnd ] {
						Game game = Game();
						unsigned char aScore = 0;
						unsigned char bScore = 0;
						for (int i = 0; i < 2; i++)
						{
							unsigned char useBrainO = i == 0 ? brainAIdx : brainBIdx;
							unsigned char useBrainX = i == 0 ? brainBIdx : brainAIdx;
							unsigned char &useScoreO = i == 0 ? aScore : bScore;
							unsigned char &useScoreX = i == 0 ? bScore : aScore;

							game.registerPlayer(new AiPlayer(aiRecurseLevels_, population_[useBrainO].brain), PieceSide::O);
							game.registerPlayer(new AiPlayer(aiRecurseLevels_, population_[useBrainX].brain), PieceSide::X);
							game.initialize();
							game.startGame();
							while (game.playTurn())
							{
								std::this_thread::yield();
							}
							int winner = game.getWinner();
							switch (winner)
							{
							case PieceSide::O + 1:
								useScoreO += 2;
								break;
							case PieceSide::X + 1:
								useScoreX += 2;
								break;
							default:
								useScoreO += 1;
								useScoreX += 1;
							}
							game.release();
						}

						scores_[brainAIdx].fetch_add(aScore);
						scores_[brainBIdx].fetch_add(bScore);

						bracketsBackbuffer[currentWinnerIndex] = aScore >= bScore ? brainAIdx : brainBIdx;
						bracketsBackbuffer[currentLoserIndex] = aScore >= bScore ? brainBIdx : brainAIdx;

						progressLock.lock();
						while ((barWidth+1) * numFinishedGames / (totalFitnessScore_ / 4.0) > currentProgressPoint)
						{
							std::cout << progressBarForeground << std::flush;
							currentProgressPoint++;
						}
						progressLock.unlock();

						// Mark this thread as available again
						bool freedInstance = false;
						while (!freedInstance)
						{
							poolLock.lock();
							if (freeThreadIndicesStart != freeThreadIndicesEnd)
							{
								freeThreadIndices[freeThreadIndicesStart] = availableThread;
								freeThreadIndicesStart = (freeThreadIndicesStart + 1) % numberOfInstances_;
								numFinishedGames++;
								freedInstance = true;
							}
							poolLock.unlock();
							std::this_thread::yield();
						}
					});


					currentWinnerIndex++;
					currentLoserIndex++;
				}
			}


			// Wait for the results for the rest of the batch
			for (unsigned char i = 0; i < numberOfInstances_; i++)
			{
				if (instances_[i].joinable())
					instances_[i].join();
			}


			// Buffer swap
			unsigned char * swap = brackets;
			brackets = bracketsBackbuffer;
			bracketsBackbuffer = swap;

			// Play through winner's tier and loser's tier next time
			roundSize = roundHalfSize;
		}

		std::cout << std::endl;

		delete[] freeThreadIndices;
		delete[] brackets;
		delete[] bracketsBackbuffer;
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
			unsigned char parentBIdx = buckets_[std::rand() % (totalFitnessScore_ - scores_[parentAIdx])]; // Weighted random but excluding parent A
			if (parentBIdx == parentAIdx)
			{
				// Can't have both parents be the same
				parentBIdx += scores_[parentAIdx];
				parentB = population_ + parentBIdx;
			}
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
			double randomStrength = (maxRandomPerGeneration_ * brainIdx) / populationSize_;

			// We don't ever mutate the first weight -- we keep it at one in order to have it as a constant reference
			for (int weightIdx = 1; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
			{
				double fraction = std::rand() / (double)RAND_MAX;
				fraction = randomStrength * (fraction * 2 - 1);
				population_[brainIdx].raw[weightIdx] += fraction;
			}
		}
	}
	void AiGeneticAlgorithm::recordFittest()
	{
		if (!recordFile_)
			return;

		for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
		{
			recordFile_ << reinterpret_cast<AiPlayer::BrainView*>(&fittest_)->raw[weightIdx];
			if (weightIdx != AiPlayer::Brain::kNumWeights - 1)
				recordFile_ << ',';
		}
		recordFile_ << std::endl;
	}
	void AiGeneticAlgorithm::initialize(unsigned char populationSizePow2, double maxRandom, int aiRecurseLevels, const char * path)
	{
		populationSize_ = 1 << populationSizePow2;
		maxRandomPerGeneration_ = maxRandom;
		aiRecurseLevels_ = aiRecurseLevels;

		// Currently there are log 2 rounds played. Each brain plays once per round which means population/2 matches and 4 points are awarded each match
		totalFitnessScore_ = populationSizePow2 * populationSize_ * 2;

		// Allocate arrays
		population_ = reinterpret_cast<AiPlayer::BrainView*>(new AiPlayer::Brain[populationSize_]);
		numberOfInstances_ = (unsigned char)std::thread::hardware_concurrency();
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

		// Opens file for writing
		if (path)
		{
			recordFile_.open(path);
			if (recordFile_)
			{
				for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
				{
					recordFile_ << AiPlayer::Brain::kWeightNames[weightIdx];
					if (weightIdx != AiPlayer::Brain::kNumWeights - 1)
						recordFile_ << ',';
				}
				recordFile_ << std::endl;
			}
		}
	}

	void AiGeneticAlgorithm::release()
	{
		recordFile_.close();

		// Deallocate arrays
		delete[] population_;
		delete[] instances_;
		delete[] scores_;
		delete[] buckets_;
	}

	void AiGeneticAlgorithm::randomize(double maxRandom)
	{
		for (int brainIdx = 0; brainIdx < populationSize_; brainIdx++)
		{
			// We don't ever mutate the first weight -- we keep it at one in order to have it as a constant reference
			for (int weightIdx = 1; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
			{
				double fraction = std::rand() / (double)RAND_MAX;
				fraction = maxRandom * (fraction * 2 - 1);
				population_[brainIdx].raw[weightIdx] = fraction;
			}
		}
	}

	void AiGeneticAlgorithm::processGeneration()
	{
		resetScores();
		calculateFitness();
		reviewFitness();
		produceOffspring();
		mutate();
		recordFittest();
	}

	AiPlayer::Brain AiGeneticAlgorithm::getFittest() const
	{
		return fittest_;
	}

}
