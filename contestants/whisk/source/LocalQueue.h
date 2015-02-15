// Copyright 2015 Paul Lange
#ifndef LOCALQUEUE_HEADER
#define LOCALQUEUE_HEADER
#include <stdint.h>
#include <limits>
#include <assert.h>
#include <malloc.h> // for MSVC specific _aligned_malloc

namespace {
	typedef uint32_t QueueIndex;
}

// A single-threaded, bounded queue
template<class T, QueueIndex length>
class LocalQueue {
	T* data;
	QueueIndex head, // index of the first element
	         tail; // index of the next empty element after the end
public:
	LocalQueue()
		: data((T*)_aligned_malloc(sizeof(T) * length, 64))
		, tail(0) {
		assert(data != 0);
		clear();
	}
	~LocalQueue() {
		_aligned_free((void*)data);
	}
	const QueueIndex max_size() const {return length;}
	void clear() {head = std::numeric_limits<QueueIndex>::max();}
	bool empty() const {return head == std::numeric_limits<QueueIndex>::max();}
	QueueIndex size() const {
		if (empty())
			return 0;
		else if (head < tail)
			return tail - head;
		else
			return length - (head - tail);
	}
	bool full() const {return head == tail;}
	bool pop(T &value) {
		if (empty())
			return false;

		value = data[head];
		head = next(head);

		// enter empty state
		if (head==tail)
			clear();

		return true;
	}
	bool push(const T value) {
		if (full())
			return false;

		// break out of empty state
		if (empty())
			head = tail;

		data[tail] = value;
		tail = next(tail);
		return true;
	}
	bool peek(T &value, QueueIndex index) const {
		if (index >= size())
			return false;
		index += head;
		index %= length;
		value = data[index];
		return true;
	}

private:
	QueueIndex next(QueueIndex pos) const {
		return ++pos % length;
	}
	void prefetch(QueueIndex index) const {
		_mm_prefetch((char*)&data[index%length], _MM_HINT_T0);
	}
};
#endif