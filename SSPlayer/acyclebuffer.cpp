#include "acyclebuffer.h"
#include <cstdlib>
#include <exception>

ACycleBuffer::ACycleBuffer(size_t n):size(n),currentSize(0)
{
	base = reinterpret_cast<char *>(malloc(n));
	if (base == nullptr)
	{
		throw std::bad_alloc();
	}
	currentStart = currentEnd = base;
}

ACycleBuffer::~ACycleBuffer()
{
	free(base);
}

bool ACycleBuffer::isFull()
{
	return currentSize==size;
}

bool ACycleBuffer::isEmpty()
{
	return currentSize==0;
}

size_t ACycleBuffer::restSpace()
{
	return size-currentSize;
}

size_t ACycleBuffer::write(const char *buffer, size_t len)
{

#ifdef THREAD_SAFE
	lock.lock();
#endif
	size_t realCount = 0;

	for (size_t i=0;!isFull()&&i<len;i++, realCount++)
	{
		*currentEnd = buffer[i];
		currentEnd++;
		currentSize++;
		if (currentEnd - base == size)
		{
			currentEnd = base;
		}
	}
#ifdef THREAD_SAFE
	lock.unlock();
#endif

	return realCount;
}

size_t ACycleBuffer::read(char *buffer, size_t len)
{

#ifdef THREAD_SAFE
	lock.lock();
#endif
	size_t realCount = 0;

	for (size_t i = 0; !isEmpty() && i<len; i++, realCount++)
	{
		buffer[i]=*currentStart;
		currentStart++;
		currentSize--;
		if (currentStart - base == size)
		{
			currentStart = base;
		}
	}
#ifdef THREAD_SAFE
		lock.unlock();
#endif

	return realCount;
}
