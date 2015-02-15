#include "point_search.h"
#include <vector>
#include "Tree.h"
#include <cstdlib>

#ifdef _MSC_VER
#define __builtin_expect(cond, val) (cond)
#endif

template <typename T>
class quick_vector
{
public:
	quick_vector()
	{
		storage = NULL;
		capacity = 0;
		size_ = 0;
	}
	
	~quick_vector()
	{
		free(storage);
	}

	T& operator[](int i)
	{
		return storage[i];
	}
	
	unsigned size()
	{
		return size_;
	}
	
	void reserve(unsigned n)
	{
		storage = (T*) realloc(storage, n * sizeof(T));
		capacity = n;
	}
	
	void resize(unsigned n)
	{
		size_ = n;
		
		if (__builtin_expect(size_ <= capacity, true))
			return;
		
		unsigned new_capacity = capacity;
		while (new_capacity < size_)
		{
			new_capacity *= 1.5;
		}
		reserve(new_capacity);
	}

private:
	T *storage;
	unsigned capacity;
	unsigned size_;
};

struct Query2
{
	// IN
	Rect rect;
	int32_t request;

	// OUT
	int32_t count;
#ifdef Q2_INDIRECT
	quick_vector<Point*>   points;
#else
	quick_vector<Point>   points;
#endif
#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
	quick_vector<int32_t> ranks;
#endif
	
	// INTERNAL
	int32_t min_rank;

	std::vector<int32_t> node_min_rank;
	std::vector<int>     node_points;
};

class Tree2: public Tree
{
public:
	Tree2(Rect span);
	
	void query2(Query2 &query);
	
	void prepare();

private:
	void query_node(Query2 &query);

	virtual void ensure_children() override;
	
	std::vector<float>   x, y;
	std::vector<int32_t> ranks4;
	std::vector<int32_t> ranks;
};
