#pragma once
#include <vector>
#include <numeric>

namespace maze_gen {

class UnionFind
{
public:
    explicit UnionFind(int size);
    int find(int x);
    void unite(int x, int y);
    bool connected(int x, int y);
    void reset(int size);

private:
    std::vector<int> _parent;
    std::vector<int> _rank;
};

}  // namespace maze_gen
