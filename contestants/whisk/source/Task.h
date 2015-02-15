// Copyright 2015 Paul Lange
#ifndef TASK_HEADER
#define TASK_HEADER
#include "TreeIndex.h"
#include <stdint.h>

struct Task {
	struct {
		uint32_t index:26, border:4, type:2;
	};
	
	enum {
		QUERY,
		ADD_ALL,
		TEST_AND_MERGE,
	//	MERGE_THREAD_RESULT,
	//	END_THREAD
	};

	//int32_t lowest_rank;
public:
	template<class T>
	void Run(T &t) const {
		switch (type) {
		case QUERY:
			t.Query(index, border);
			break;
		case ADD_ALL:
			t.AddAll(index);
			break;
		case TEST_AND_MERGE:
			t.TestAndMerge(index);
			break;
		}
	}
	template<class T>
	void Prefetch(const T &t, const int level) const {
		switch (type) {
		case QUERY:
			t.PrefetchQuery(index, level);
			break;
		case ADD_ALL:
			t.PrefetchAddAll(index, level);
			break;
		case TEST_AND_MERGE:
			t.PrefetchTestAndMerge(index, level);
			break;
		}
	}
	static Task TestAndMerge(TreeIndex index) {
		Task t;
		t.type = TEST_AND_MERGE;
		t.index = index;
//		t.lowest_rank = rank;
		return t;
	}
	static Task AddAll(TreeIndex index) {
		Task t;
		t.type = ADD_ALL;
		t.index = index;
//		t.lowest_rank = rank;
		return t;
	}
	static Task Query(TreeIndex index, int border = 0) {
		Task t;
		t.type = QUERY;
		t.index = index;
		t.border = border;
//		t.lowest_rank = rank;
		return t;
	}
};
#endif