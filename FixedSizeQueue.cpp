#include "FixedSizeQueue.h"


FixedSizeQueue::FixedSizeQueue(void)
{


}

FixedSizeQueue::FixedSizeQueue(size_t queueSize)
{
	this->maxSize = queueSize;
}

void FixedSizeQueue::push(double val)
{
	if (q.size() >= maxSize)
	{
		q.pop();
	}

	this->q.push(val);
}
	
void FixedSizeQueue::pop()
{
	this->q.pop();
}
	
size_t FixedSizeQueue::size()
{
	return this->q.size();
}
	
double FixedSizeQueue::front()
{
	return this->q.front();
}
	
double FixedSizeQueue::back()
{
	return this->q.back();
}

size_t FixedSizeQueue::getMaxSize()
{
	return this->maxSize;
}

double FixedSizeQueue::getSum()
{
	double sum = 0;
	int size = q.size();
	for (int i = 0; i < size; i++)
	{
		double val = q.front();
		sum += val;
		q.pop();
		q.push(val);
	}

	return sum;
}



FixedSizeQueue::~FixedSizeQueue(void)
{
}
