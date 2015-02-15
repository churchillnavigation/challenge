// 
// SharedQueue.h
// 01/22/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A concurrent queue class for use with thread pools.
// 
#include <vector>
#include <mutex>
#include <condition_variable>


template <typename Element>
class SharedQueue
{
  public:
    typedef Element element_type;
    typedef Element& reference;
    typedef const Element& const_reference;
    typedef size_t size_type;

  public:
    ~SharedQueue();

    void push(typename SharedQueue::const_reference item);
    typename SharedQueue::element_type pop();

    typename SharedQueue::size_type size() const;
    bool empty() const;
    void clear();

  private:
    std::vector<element_type> mQueue;
    std::mutex mMutex;
    std::condition_variable mNotEmpty;
};


//******************************************************************
//*  Template function implementation:
//******************************************************************

template <typename Element>
SharedQueue<Element>::~SharedQueue()
{
  clear();
}


template <typename Element>
void SharedQueue<Element>::push(typename SharedQueue<Element>::const_reference item)
{
  std::unique_lock<std::mutex> lock(mMutex);
  mQueue.push_back(item);
  mNotEmpty.notify_one();
}


template <typename Element>
typename SharedQueue<Element>::element_type SharedQueue<Element>::pop()
{
  std::unique_lock<std::mutex> lock(mMutex);
  
  // Sit & wait for an element to be available:
  // Note:  We're purposely NOT using our own SharedQueue::empty()
  //        function here to avoid requiring a recursive mutex.
  if (mQueue.empty()) {
    mNotEmpty.wait(lock, [this]() { return !mQueue.empty(); });
  }

  element_type val = mQueue.back();
  mQueue.pop_back();
  return val;
}


template <typename Element>
typename SharedQueue<Element>::size_type SharedQueue<Element>::size() const
{
  std::unique_lock<std::mutex> lock(mMutex);
  return mQueue.size();
}


template <typename Element>
bool SharedQueue<Element>::empty() const
{
  std::unique_lock<std::mutex> lock(mMutex);
  return mQueue.empty();
}


template <typename Element>
void SharedQueue<Element>::clear()
{
  std::unique_lock<std::mutex> lock(mMutex);
  mQueue.clear();
}
