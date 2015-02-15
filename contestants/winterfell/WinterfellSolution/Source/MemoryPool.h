#pragma once

#include <stdint.h>

class MemoryPool
{
public:
	MemoryPool(int inSizeInBytes);
	~MemoryPool();

	template<typename T, typename ...Args>
	T* Alloc(const Args&... inArgs)
	{
		void* mem = AllocInternal( sizeof(T), __alignof(T) );
		return new (mem) T(inArgs...);
	}

	void Reset();

private:
	void* AllocInternal(size_t inNumBytes, size_t inAlignment);

	void*		mStart;
	void*		mEnd;
	void*		mCursor;
	MemoryPool*	mNext;
	const int	mSizeInBytes;

};
