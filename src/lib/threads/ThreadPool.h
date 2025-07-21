//
// Created by michi on 7/21/25.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "../base/base.h"
#include "Thread.h"
#include <functional>

class PoolWorkerThread;

class ThreadPool {
public:
	ThreadPool(int num_threads = -1);
	~ThreadPool();

	void run(int n, std::function<void(int)> f);

	Array<PoolWorkerThread*> threads;
};


#endif //THREADPOOL_H
