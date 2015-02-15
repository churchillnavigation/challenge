// 
// Barrier.cpp
// 01/20/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A barrier class for synchronizing execution of multiple threads.
// Code is based on chapter 7 of Butenhof's "Programming with POSIX
// Threads".
// 
#include "Barrier.h"


Barrier::Barrier(int threadCount)
  : mCycle(0)
  , mThreadCount(threadCount)
  , mRemaining(threadCount)
{
}


int Barrier::Wait()
{
  std::unique_lock<std::mutex> lock(mMutex);

  const int cycle = mCycle;
  
  if (--mRemaining > 0) {
	// We're still waiting for at least 1 other thread to arrive...
    while (mCycle == cycle) {
      mCV.wait(lock);
	}
    return 0;
  }
  else {
	// The last thread has arrived.  Release the whole gang:
    ++mCycle;
    mRemaining = mThreadCount;
    mCV.notify_all();
    return 1;
  }
}
