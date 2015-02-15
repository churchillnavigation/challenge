// Copyright 2015 Paul Lange
#ifndef BINARYTREE_HEADER
#define BINARYTREE_HEADER
#include "TreeDimensions.h"
#include "TreeIndex.h"
#include <malloc.h> // for MSVC specific _aligned_malloc
#include <memory>
#include <assert.h>
#include <type_traits>
#include <intrin.h>

namespace {
	// returns 0 for empty classes
	template <class T>
	const size_t TrueSizeOf() {
		return (std::is_empty<T>::value)? 0: sizeof(T);
	}
	// Byte size of tree
	template <class Node_t, class Leaf_t>
	const size_t SizeOfTree(const TreeDimensions dimensions) {
		return dimensions.nonleaves() * TrueSizeOf<Node_t>() + dimensions.leaves() * TrueSizeOf<Leaf_t>();
	}
	void AlignedDeleter(char *ptr) {
		_aligned_free(reinterpret_cast<void*>(ptr));
	}
}

template <class Node_t, class Leaf_t>
class BinaryTree {
	std::unique_ptr<char, void(*)(char*)> buffer;
	const TreeDimensions dimensions;

private:
	// Doesn't retain the data
	void ResizeBuffer(const size_t bytes) {
		buffer.reset( reinterpret_cast<char*>(_aligned_malloc(bytes, 64)) );
		//if (buffer.get() == NULL)
		//	throw std::bad_alloc();
	}

public:
	BinaryTree(const TreeDimensions dimensions)
		: buffer(NULL, AlignedDeleter)
		, dimensions(dimensions) {
		ResizeBuffer( SizeOfTree<Node_t,Leaf_t>(dimensions) );
	}
	TreeDimensions Dimensions() const { return dimensions; }
#if 0
	Iterator Root() const {
		Iterator it;
		it.byte_ptr = buffer.get();
		it.dimensions = dimensions;
		return it;
	}
#endif
	Leaf_t& getLeaf(const TreeIndex index) const {
		assert(index.isLeaf(dimensions));
		char *byte_ptr = buffer.get();
		byte_ptr += TrueSizeOf<Node_t>() * dimensions.nonleaves();
		byte_ptr += TrueSizeOf<Leaf_t>() * (index - dimensions.nonleaves());
		return *reinterpret_cast<Leaf_t*>(byte_ptr);
	}
	Node_t& getNode(const TreeIndex index) const {
		//assert(!index.isLeaf(dimensions));
		char *byte_ptr = buffer.get();
		byte_ptr += TrueSizeOf<Node_t>() * index;
		return *reinterpret_cast<Node_t*>(byte_ptr);
	}
	void PrefetchNode(const TreeIndex index, const int cache_level) const {
		switch (cache_level) {
		case 1:
			_mm_prefetch((char*)&getNode(index), _MM_HINT_T0);
			break;
		case 2:
			_mm_prefetch((char*)&getNode(index), _MM_HINT_T1);
			break;
		}
	}
	void PrefetchLeaf(const TreeIndex index, const int cache_level) const {
		switch (cache_level) {
		case 1:
			_mm_prefetch((char*)&getLeaf(index), _MM_HINT_T0);
			break;
		case 2:
			_mm_prefetch((char*)&getLeaf(index), _MM_HINT_T1);
			break;
		}
	}
};

#endif
