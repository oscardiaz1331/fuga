#include "maze_gen/union_find.hpp"

namespace maze_gen {

UnionFind::UnionFind(int size) {
	_parent.resize(size);
	_rank.resize(size, 0);
	std::iota(_parent.begin(), _parent.end(), 0);
}

int UnionFind::find(int x) {
	if (_parent[x] != x) {
		_parent[x] = find(_parent[x]);
	}
	return _parent[x];
}

void UnionFind::unite(int x, int y) {
	int root_x = find(x);
	int root_y = find(y);
	if (root_x == root_y) return;
	if (_rank[root_x] < _rank[root_y]) {
		_parent[root_x] = root_y;
	}
	else {
		_parent[root_y] = root_x;
		if (_rank[root_x] == _rank[root_y]) {
			++_rank[root_x];
		}
	}
}

bool UnionFind::connected(int x, int y) {
	return find(x) == find(y);
}

void UnionFind::reset(int size) {
	_parent.resize(size);
	_rank.resize(size, 0);
	std::iota(_parent.begin(), _parent.end(), 0);
}

}  // namespace maze_gen
