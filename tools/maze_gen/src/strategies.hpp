#pragma once
#include <cstdint>
#include <vector>
#include "maze_strategy.hpp"

namespace maze_gen
{

// One class per algorithm. docs/algoritmos.md explains how each of them works.

class DFSMazeStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class AldousBroderStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class CellularAutomatonStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class EllersStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class BinaryTreeStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class GrowingTreeStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class HuntAndKillStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class KruskalsStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class PrimsStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class RecursiveDivisionStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class SidewinderStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class WilsonsStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

class HoustonStrategy : public MazeStrategy
{
public:
  void generate(MazeGrid & grid, Random & rng, const MazeStrategyOptions & options) override;
};

// The loop of Wilson's algorithm: add loop-erased random walks to the cells already
// marked in in_tree until every cell is part of the maze. Shared with Houston.
void wilson_complete(
  MazeGrid & grid, Random & rng, std::vector<uint8_t> & in_tree, int in_tree_count);

}  // namespace maze_gen
