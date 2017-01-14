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

						progressLock.lock();
						scores_[brainAIdx] += aScore;
						scores_[brainBIdx] += bScore;

						bracketsBackbuffer[currentWinnerIndex] = aScore >= bScore ? brainAIdx : brainBIdx;
						bracketsBackbuffer[currentLoserIndex] = aScore >= bScore ? brainBIdx : brainAIdx;

						
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
	void AiGeneticAlgorithm::sortIndices(unsigned char * indices, unsigned const char * data, int count) const
	{
		if (count < 2) // We don't need to do anything if there is only one or none elements
			return;

		// Quicksort
		int pivotIndex = std::rand() % count;

		// Partition
		for (int i = 0; i < count; i++)
		{
			if (i < pivotIndex && data[indices[i]] > data[indices[pivotIndex]])
			{
				unsigned char swap = indices[i];
				indices[i] = indices[pivotIndex - 1];
				indices[pivotIndex - 1] = indices[pivotIndex];
				indices[pivotIndex] = swap;
				pivotIndex--;
				i--;
			}
			else
			if (i > pivotIndex && data[indices[i]] < data[indices[pivotIndex]])
			{
				unsigned char swap = indices[i];
				indices[i] = indices[pivotIndex + 1];
				indices[pivotIndex + 1] = indices[pivotIndex];
				indices[pivotIndex] = swap;
				pivotIndex++;
			}
		}

		sortIndices(indices, data, pivotIndex);
		sortIndices(indices + pivotIndex + 1, data + pivotIndex + 1,count - pivotIndex - 1);
	}
	void AiGeneticAlgorithm::reviewFitness()
	{
		unsigned char *sortedIndices = new unsigned char[populationSize_];
		for (unsigned char i = 0; i < populationSize_; i++) 
		{ 
			sortedIndices[i] = i; 
		}
		sortIndices(sortedIndices, scores_, populationSize_);

		int currentBucketIdx = 0;
		for (unsigned char i = 0; i < populationSize_; i++)
		{
			unsigned char actualIndex = sortedIndices[populationSize_ - i - 1];
			for (int fill = 0; fill < scores_[actualIndex]; fill++)
			{
				buckets_[currentBucketIdx++] = actualIndex;
			}
		}

		fittest_ = population_[sortedIndices[populationSize_ - 1]].brain;
		delete[] sortedIndices;
	}
	void AiGeneticAlgorithm::mate(const AiPlayer::BrainView &parentA, const AiPlayer::BrainView &parentB, AiPlayer::BrainView &outOffspring) const
	{
		for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
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

		int survivedCount = (int)(totalFitnessScore_ * topPercent_);

		// Create offspring
		for (int i = 1; i < populationSize_; i++)
		{
			if (scores_[buckets_[0]] >= survivedCount)
			{
				// If only one individual survived, the offspring is a clone
				offSpring[i] = population_[buckets_[0]].brain;
			}
			else
			{
				AiPlayer::BrainView *parentA;
				AiPlayer::BrainView *parentB;

				unsigned char parentAIdx = buckets_[std::rand() % survivedCount]; // Weighted random
				parentA = population_ + parentAIdx;

				unsigned char parentBIdx = buckets_[std::rand() % (survivedCount - scores_[parentAIdx])]; // Weighted random but excluding parent A
				if (parentBIdx == parentAIdx)
				{
					// Can't have both parents be the same
					parentBIdx += scores_[parentAIdx];
					parentB = population_ + parentBIdx;
				}
				parentB = population_ + parentBIdx;

				mate(*parentA, *parentB, *reinterpret_cast<AiPlayer::BrainView*>(offSpring + i));
			}
		}

		// Fill new generation
		for (int i = 0; i < populationSize_; i++)
		{
			population_[i].brain = offSpring[i];
		}

		delete[] offSpring;
	}
	void AiGeneticAlgorithm::normalize(AiPlayer::BrainView & brain, double magnitude) const
	{
		double total = 0;
		for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
		{
			total += std::abs(brain.raw[weightIdx]);
		}
		double average = total / AiPlayer::Brain::kNumWeights;

		if (average == 0)
			return;

		double scale = magnitude / average;
		for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
		{
			brain.raw[weightIdx] *= scale;
		}
	}
	void AiGeneticAlgorithm::mutate()
	{
		for (int brainIdx = 1; brainIdx < populationSize_; brainIdx++)
		{
			double randomStrength = (maxRandomPerGeneration_ * brainIdx) / (populationSize_-1);

			for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
			{
				double fraction = std::rand() / (double)RAND_MAX;
				fraction = randomStrength * (fraction * 2 - 1);
				population_[brainIdx].raw[weightIdx] += fraction;
			}
			normalize(population_[brainIdx]);
		}
	}
	void AiGeneticAlgorithm::recordFittest()
	{
		if (!recordFile_)
			return;

		AiPlayer::BrainView* view = reinterpret_cast<AiPlayer::BrainView*>(&fittest_);
		for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
		{
			recordFile_ << view->raw[weightIdx];
			if (weightIdx != AiPlayer::Brain::kNumWeights - 1)
				recordFile_ << ',';
		}
		recordFile_ << std::endl;
	}
	void AiGeneticAlgorithm::initialize(unsigned char populationSizePow2, int aiRecurseLevels, double maxRandomPerGeneration, double topPercent, const char * outputCsvPath)
	{
		populationSize_ = 1 << populationSizePow2;
		maxRandomPerGeneration_ = maxRandomPerGeneration;
		aiRecurseLevels_ = aiRecurseLevels;
		topPercent_ = topPercent;

		// Currently there are log 2 rounds played. Each brain plays once per round which means population/2 matches and 4 points are awarded each match
		totalFitnessScore_ = populationSizePow2 * populationSize_ * 2;

		// Allocate arrays
		population_ = reinterpret_cast<AiPlayer::BrainView*>(new AiPlayer::Brain[populationSize_]);
		numberOfInstances_ = (unsigned char)std::thread::hardware_concurrency();
		instances_ = new std::thread[numberOfInstances_];
		scores_ = new unsigned char[populationSize_];
		buckets_ = new unsigned char[totalFitnessScore_];

		// Seed random with current time
		std::srand((unsigned int)time(0));

		// Initialize population with current default brain
		for (int i = 0; i < populationSize_; i++)
		{
			population_[i].brain = AiPlayer::kDefaultBrain;
		}
		fittest_ = population_[0].brain;

		// Opens file for writing
		if (outputCsvPath)
		{
			recordFile_.open(outputCsvPath);
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

	void AiGeneticAlgorithm::randomize()
	{
		for (int brainIdx = 0; brainIdx < populationSize_; brainIdx++)
		{
			for (int weightIdx = 0; weightIdx < AiPlayer::Brain::kNumWeights; weightIdx++)
			{
				double fraction = std::rand() / (double)RAND_MAX;
				fraction = (fraction * 2 - 1);
				population_[brainIdx].raw[weightIdx] = fraction;
			}
			normalize(population_[brainIdx]);
		}
	}

	void AiGeneticAlgorithm::processGeneration()
	{
		mutate();
		resetScores();
		calculateFitness();
		reviewFitness();
		produceOffspring();
		recordFittest();
	}

	AiPlayer::Brain AiGeneticAlgorithm::getFittest() const
	{
		return fittest_;
	}

}
