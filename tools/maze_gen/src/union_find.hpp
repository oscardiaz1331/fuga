#pragma once
#include <vector>

namespace maze_gen
{

// Disjoint sets over the integers 0..size-1
class UnionFind
{
public:
  explicit UnionFind(int size);
  int find(int x);
  void unite(int x, int y);
  bool connected(int x, int y);

private:
  std::vector<int> parent_;
  std::vector<int> rank_;
};

}  // namespace maze_gen
