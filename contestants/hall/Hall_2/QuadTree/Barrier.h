// 
// Barrier.h
// 01/20/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A barrier class for synchronizing execution of multiple threads.
// Code is based on chapter 7 of Butenhof's "Programming with POSIX
// Threads".
// 
#include <mutex>
#include <condition_variable>


class Barrier
{
  public:
    Barrier(int threadCount);
	
    int Wait();

  private:
	std::mutex mMutex;
    std::condition_variable mCV;
    int mCycle;
    int mThreadCount;
    int mRemaining;
};

