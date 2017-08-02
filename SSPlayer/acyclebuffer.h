#pragma once

#define THREAD_SAFE

#ifdef THREAD_SAFE
#include <mutex>
#endif

class ACycleBuffer final
{
public:
	ACycleBuffer(size_t);
	ACycleBuffer(const ACycleBuffer &) = delete;
	ACycleBuffer(ACycleBuffer &&) = delete;
	ACycleBuffer& operator=(const ACycleBuffer &) = delete;
	~ACycleBuffer();
	bool isFull();
	bool isEmpty();
	size_t restSpace();
	size_t write(const char *, size_t);
	size_t read(char *, size_t);
private:
	char *base;
	char *currentStart;
	char *currentEnd;
	size_t size;
	size_t currentSize;
#ifdef THREAD_SAFE
	std::mutex lock;
#endif
};