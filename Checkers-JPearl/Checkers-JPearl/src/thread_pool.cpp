#include "thread_pool.h"

namespace checkers
{
	void ThreadPool::lookForWork()
	{
		bool threadRunning = true;
		while (threadRunning)
		{
			bool foundJob = false;
			std::function<void(void)> job;
			poolMutex_.lock();
			if (!funcs_.empty())
			{
				job = funcs_.front();
				funcs_.pop_front();
				numThreadsWorking_++;
				foundJob = true;
			}
			poolMutex_.unlock();


			if (foundJob)
			{
				job();
				poolMutex_.lock();
				numThreadsWorking_--;
				poolMutex_.unlock();
			}

			if (!foundJob && !isRunning_)
			{
				threadRunning = false;
			}
			std::this_thread::yield();
		}
	}

	void ThreadPool::initialize()
	{
		numThreads_ = (unsigned char) std::thread::hardware_concurrency();
		threads_ = new std::thread[numThreads_];
		isRunning_ = true;
		numThreadsWorking_ = 0;
		
		for (unsigned char i = 0; i < numThreads_; i++)
		{
			threads_[i] = std::thread([this] {lookForWork(); });
		}
	}
	void ThreadPool::release()
	{
		isRunning_ = false;
		for (unsigned char i = 0; i < numThreads_; i++)
		{
			if (threads_[i].joinable())
				threads_[i].join();
		}

		funcs_.clear();
		if (threads_)
		{
			delete [] threads_;
			threads_ = nullptr;
		}
	}
	void ThreadPool::addJob(const std::function<void(void)> &job)
	{
		poolMutex_.lock();
		funcs_.push_back(job);
		poolMutex_.unlock();
	}
	void ThreadPool::join()
	{
		while (isWorking())
		{
			std::this_thread::yield();
		}
	}
	bool ThreadPool::isWorking()
	{
		poolMutex_.lock();
		bool result = numThreadsWorking_ > 0 || !funcs_.empty();
		poolMutex_.unlock();
		return result;
	}
}
