#include "ai_genetic_algorithm.h"

#include <fstream>

#include <mutex>
#include <iostream>

#include "elimination_tournament.h"
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
		// Show a progress bar
		const int barWidth = 50;
		const char progressBarBackground = (unsigned char)176;
		const char progressBarForeground = (unsigned char)178;
		std::mutex progressLock;
		for (int i = 0; i < barWidth; i++)
			std::cout << progressBarBackground;
		std::cout << '\r' << std::flush;
		int currentProgressPoint = 0; currentProgressPoint;
		int numFinishedGames = 0; numFinishedGames;

		
		
		// Run a tournament
		unsigned char expectedSurvivors = (unsigned char)std::ceil(populationSize_ * topPercent_);
		EliminationTournament tourn;
		tourn.initialize(populationSize_, (expectedSurvivors+1)/2);
		tourn.run<AiPlayer::BrainView>(population_, [this, &progressLock, &numFinishedGames, &currentProgressPoint, progressBarForeground, barWidth, tourn](AiPlayer::BrainView& teamA, AiPlayer::BrainView& teamB) -> AiPlayer::BrainView& {
			Game game = Game();
			unsigned char aScore = 0;
			unsigned char bScore = 0;
			for (int i = 0; i < 2; i++)
			{
				AiPlayer::BrainView& useBrainO = i == 0 ? teamA : teamB;
				AiPlayer::BrainView& useBrainX = i == 0 ? teamB : teamA;
				unsigned char &useScoreO = i == 0 ? aScore : bScore;
				unsigned char &useScoreX = i == 0 ? bScore : aScore;

				game.registerPlayer(new AiPlayer(aiRecurseLevels_, useBrainO.brain), PieceSide::O);
				game.registerPlayer(new AiPlayer(aiRecurseLevels_, useBrainX.brain), PieceSide::X);
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
			numFinishedGames++;
			while ((barWidth + 1) * numFinishedGames / tourn.getMaxNumMatches() > currentProgressPoint)
			{
				std::cout << progressBarForeground << std::flush;
				currentProgressPoint++;
			}
			progressLock.unlock();

			return aScore >= bScore ? teamA : teamB;
		});
		std::cout << std::endl;

		float scoreForRank = 1;
		int numPlaces = tourn.getNumResolvedPlaces();
		for (int place = 1; place <= numPlaces; place++ )
		{
			int numInRank = 0;
			const unsigned int * indicesInRank = tourn.getTeamsAtPlace(place, numInRank);
			for (int i = 0; i < numInRank; i++)
			{
				scores_[indicesInRank[i]] = scoreForRank / numInRank;
			}
			scoreForRank /= 2;
			if (scoreForRank == 0)
				break;
		}
	}
	void AiGeneticAlgorithm::sortIndices(unsigned int * indices, const float * data, int count) const
	{

		for (int i = 0; i < count; i++)
		{
			int leastIndex = i;
			for (int j = i + 1; j < count; j++)
			{
				if (data[indices[j]] < data[indices[leastIndex]])
					leastIndex = j;
			}
			unsigned int swap = indices[leastIndex];
			indices[leastIndex] = indices[i];
			indices[i] = swap;
		}
		/*
		if (count < 2) // We don't need to do anything if there is only one or none elements
			return;

		// Quicksort
		int pivotIndex = std::rand() % count;

		// Partition
		for (int i = 0; i < count; i++)
		{
			if (i < pivotIndex && data[indices[i]] > data[indices[pivotIndex]])
			{
				unsigned int swap = indices[i];
				indices[i] = indices[pivotIndex - 1];
				indices[pivotIndex - 1] = indices[pivotIndex];
				indices[pivotIndex] = swap;
				pivotIndex--;
				i--;
			}
			else
			if (i > pivotIndex && data[indices[i]] < data[indices[pivotIndex]])
			{
				unsigned int swap = indices[i];
				indices[i] = indices[pivotIndex + 1];
				indices[pivotIndex + 1] = indices[pivotIndex];
				indices[pivotIndex] = swap;
				pivotIndex++;
			}
		}

		sortIndices(indices, data, pivotIndex);
		sortIndices(indices + pivotIndex + 1, data + pivotIndex + 1,count - pivotIndex - 1);
		*/
	}
	void AiGeneticAlgorithm::reviewFitness()
	{
		for (unsigned int i = 0; i < populationSize_; i++) 
		{ 
			sortedIndices_[i] = i; 
		}
		sortIndices(sortedIndices_, scores_, populationSize_);

		totalFitnessScore_ = 0;
		for (unsigned int i = 0; i < populationSize_; i++)
		{
			unsigned int actualIndex = sortedIndices_[populationSize_ - i - 1];
			totalFitnessScore_ += scores_[actualIndex];
			sortedCumulativeScores_[i] = totalFitnessScore_;
		}

		fittest_ = population_[sortedIndices_[populationSize_ - 1]].brain;
	}
	unsigned int AiGeneticAlgorithm::findParent() const
	{
		float scoreWeight = totalFitnessScore_ * (std::rand() / (float) RAND_MAX );
		for (unsigned int i = 0; i < populationSize_; i++)
		{
			if (sortedCumulativeScores_[i] > scoreWeight)
				return sortedIndices_[populationSize_ - i - 1];
		}

		return populationSize_;
	}
	unsigned int AiGeneticAlgorithm::findParent(unsigned int toExclude) const
	{
		if (scores_[toExclude] >= totalFitnessScore_)
			return toExclude;

		float scoreWeight = (totalFitnessScore_- scores_[toExclude]) * (std::rand() / (float)RAND_MAX);
		for (unsigned int i = 0; i < populationSize_; i++)
		{
			if (sortedIndices_[populationSize_ - i - 1] == toExclude)
			{
				scoreWeight += scores_[toExclude];
				break;
			}

			if (sortedCumulativeScores_[i] > scoreWeight)
				return sortedIndices_[populationSize_ - i - 1];
		}

		return populationSize_;
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

		// Create offspring
		for (unsigned int i = 1; i < populationSize_; i++)
		{
			unsigned int parentAIdx = findParent();
			unsigned int parentBIdx = findParent(parentAIdx);

			AiPlayer::BrainView *parentA = population_ + parentAIdx;
			AiPlayer::BrainView *parentB = population_ + parentBIdx;

			mate(*parentA, *parentB, *reinterpret_cast<AiPlayer::BrainView*>(offSpring + i));
		}

		// Fill new generation
		for (unsigned int i = 0; i < populationSize_; i++)
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
		for (unsigned int brainIdx = 1; brainIdx < populationSize_; brainIdx++)
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

		// Allocate arrays
		population_ = reinterpret_cast<AiPlayer::BrainView*>(new AiPlayer::Brain[populationSize_]);
		scores_ = new float[populationSize_];
		sortedIndices_ = new unsigned int[populationSize_];
		sortedCumulativeScores_ = new float[populationSize_];

		// Seed random with current time
		std::srand((unsigned int)time(0));

		// Initialize population with current default brain
		for (unsigned int i = 0; i < populationSize_; i++)
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
		delete[] scores_;
		delete[] sortedIndices_;
		delete[] sortedCumulativeScores_;
	}

	void AiGeneticAlgorithm::randomize()
	{
		for (unsigned int brainIdx = 0; brainIdx < populationSize_; brainIdx++)
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
