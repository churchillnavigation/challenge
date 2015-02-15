// Copyright 2015 Paul Lange
#ifndef TREE_DIMENSIONS_H
#define TREE_DIMENSIONS_H
#include <assert.h>

// Calculates dimensions of a perfect binary tree
class TreeDimensions {
	size_t _height;
public:
	TreeDimensions(const size_t height): _height(height) {}
	TreeDimensions(){}
	TreeDimensions(const TreeDimensions &copy): _height(copy._height) {}
	TreeDimensions& operator=(const TreeDimensions &copy){_height = copy._height; return *this;}
	size_t height() const {return _height;}
	size_t leaves() const {return (size_t)1 << _height;}
	size_t nodes() const {return ((size_t)1 << (_height + 1)) - 1;}
	size_t nonleaves() const {return ((size_t)1 << _height) - 1;}
	bool isLeaf() const {return _height==0;}
	TreeDimensions child() const {
		assert(!isLeaf());
		return TreeDimensions(_height - 1);
	}
	static TreeDimensions fromHeight(size_t height) {return TreeDimensions(height);}
	// could use the BSR instruction instead of a loop, _BitScanReverse64
	static TreeDimensions fromLeaves(size_t leaves) {
		if (leaves<=1)
			return TreeDimensions(0);
		size_t height = 1;
		leaves -= 1;
		while (leaves/=2) ++height;
		return TreeDimensions(height);
	}
	static TreeDimensions fromNodes(size_t leaves) {
		size_t height = 0;
		while (leaves/=2) ++height;
		return TreeDimensions(height);
	}
};
#endif