// Copyright 2015 Paul Lange
#ifndef TREEINDEX_HEADER
#define TREEINDEX_HEADER
#include "TreeDimensions.h"
#include <assert.h>
#include <intrin.h>

// Breadth search based binary tree index
struct TreeIndex {
	typedef size_t index_t;
	index_t index;

	TreeIndex(const index_t offset)
		:index(offset)
	{}
#if 0
	template<class T>
	TreeIndex(const T* base_ptr, const T* position_ptr)
		:index(position_ptr - base_ptr)
	{
		assert(base_ptr <= position_ptr);
	}
#endif
	// The level of the index. The root node being level 0.
	unsigned long Level() const {
		unsigned long bit_index;
		_BitScanReverse64(&bit_index, index + 1);
		return bit_index;
	}
	// Position within the level
	index_t LevelOffset() const {
		index_t offset = index + 1;
		_bittestandreset64(reinterpret_cast<__int64*>(&offset), Level());
		return offset;
	}
	TreeIndex LeftChild() const {
		return TreeIndex(index * 2 + 1);
	}
	TreeIndex RightChild() const {
		return TreeIndex(index * 2 + 2);
	}
	TreeIndex Parent() const {
		assert(index != 0 && "Root has no parent");
		return TreeIndex((index - 1) / 2);
	}
	TreeIndex& operator ++() {
		++index;
		return *this;
	}
	TreeIndex& operator --() {
		--index;
		return *this;
	}
	operator index_t() const {
		return index;
	}
	static TreeIndex At(const index_t level, const index_t offset) {
		return TreeIndex((1u << level) - 1 + offset);
	}
	static TreeIndex Root() {
		return TreeIndex(0);
	}
	// Returns the dimensions of the branch as if the index was root
	TreeDimensions Branch(const TreeDimensions base) const {
		return TreeDimensions::fromHeight(base.height() - Level());
	}
	bool isLeaf(const TreeDimensions base) const {
		return Branch(base).isLeaf();
	}
};
#endif