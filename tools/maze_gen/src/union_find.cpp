#include "union_find.hpp"
#include <numeric>

namespace maze_gen
{

UnionFind::UnionFind(int size)
: parent_(size), rank_(size, 0)
{
  std::iota(parent_.begin(), parent_.end(), 0);
}

int UnionFind::find(int x)
{
  if (parent_[x] != x) {
    parent_[x] = find(parent_[x]);
  }
  return parent_[x];
}

void UnionFind::unite(int x, int y)
{
  int root_x = find(x);
  int root_y = find(y);
  if (root_x == root_y) {
    return;
  }
  if (rank_[root_x] < rank_[root_y]) {
    parent_[root_x] = root_y;
  } else {
    parent_[root_y] = root_x;
    if (rank_[root_x] == rank_[root_y]) {
      ++rank_[root_x];
    }
  }
}

bool UnionFind::connected(int x, int y)
{
  return find(x) == find(y);
}

}  // namespace maze_gen
