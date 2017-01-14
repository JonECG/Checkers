#pragma once

#include <functional>
#include <thread>
#include <list>
#include <mutex>

namespace checkers
{
	class ThreadPool
	{
		std::list<std::function<void(void)>> funcs_;
		std::thread *threads_;
		unsigned char numThreads_;
		unsigned char numThreadsWorking_;
		bool isRunning_;
		std::mutex poolMutex_;

		void lookForWork();
	public:
		void initialize();
		void release();

		void addJob(const std::function<void(void)>& job);
		void join();

		bool isWorking();
	};
}
