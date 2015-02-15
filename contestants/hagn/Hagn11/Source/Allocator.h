#pragma once

#include <vector>

template <class T> class Allocator {
private:
	std::vector<T*> _cache;
	int _cacheSize = 100032;
public:
	inline T * GetNext()
	{
		if (_cache.size() == 0) {
			_cache.reserve(_cacheSize);
			for (int i = 0; i < _cacheSize; i++) {
				T* newNode = new T();
				_cache.push_back(newNode);
			}
		}
		T* lastElement = _cache.back();
		_cache.pop_back();
		return lastElement;
	}


	inline void Return(T * element)
	{
		new(element) T();
		_cache.push_back(element);
	}


	inline ~Allocator()
	{
		for (T* node : _cache) {
			delete node;
		}
	}
};