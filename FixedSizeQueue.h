#pragma once

#include <queue>

/*
	Queue that has a fixed size. Implemented mainly for use as a queue for computing FPS

*/

class FixedSizeQueue
{
private:
	size_t maxSize;
	std::queue<double> q;

public:
	FixedSizeQueue(void);
	FixedSizeQueue(size_t queueSize);
	~FixedSizeQueue(void);

	void push(double val);
	void pop();
	size_t size();
	double front();
	double back();

	double getSum();

	size_t getMaxSize();
};

