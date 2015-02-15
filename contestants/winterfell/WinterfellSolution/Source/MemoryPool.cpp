#include "MemoryPool.h"

#include <algorithm>
#include "Switches.h"


MemoryPool::MemoryPool(int inSizeInBytes)
	: mNext(nullptr)
	, mSizeInBytes(inSizeInBytes)
{
#ifdef _FORCE_ALIGNMENT
	mStart = mCursor = _aligned_malloc(inSizeInBytes, 64);
#else
	mStart = mCursor = malloc(inSizeInBytes);
#endif 

	mEnd = (char*)(mStart) + inSizeInBytes;
}

MemoryPool::~MemoryPool()
{
	delete mNext;

#ifdef _FORCE_ALIGNMENT
	_aligned_free(mStart);
#else
	free(mStart);
#endif 

	mStart = mCursor = mEnd = mNext = nullptr;
}

void* MemoryPool::AllocInternal(size_t inNumBytes, size_t inAlignment)
{
#ifdef _FORCE_ALIGNMENT
	char* memOut = (char*) ((reinterpret_cast<long long>(mCursor) + (inAlignment-1)) & ~(inAlignment-1));
#else
	char* memOut = (char*) (mCursor);
#endif

	void* nextCursor = memOut + inNumBytes;
	if (nextCursor <= (char*) (mEnd))
	{
		mCursor = nextCursor;
		return memOut;
	}
	else
	{
		// Alloc a new mem pool.
		if (mNext == nullptr)
		{
			int remaining = (int) ((char*) (mEnd) -(char*) (mCursor));
			int length = std::max((int) inNumBytes, mSizeInBytes);
			mNext = new MemoryPool(length);
		}

		return mNext->AllocInternal(inNumBytes, inAlignment);
	}
}

void MemoryPool::Reset()
{
	mCursor = mStart;

	if (nullptr != mNext)
	{
		mNext->Reset();
	}
}
