#include "algorithm_registry.hpp"
#include <array>
#include <stdexcept>
#include <string>
#include "strategies.hpp"

namespace maze_gen
{

namespace
{

template<class Strategy>
std::unique_ptr<MazeStrategy> make()
{
  return std::make_unique<Strategy>();
}

const std::array<AlgorithmInfo, 13> kAlgorithms = {{
  {MazeAlgorithm::DFS, "dfs", make<DFSMazeStrategy>, true},
  {MazeAlgorithm::AldousBroder, "aldous_broder", make<AldousBroderStrategy>, true},
  {MazeAlgorithm::CellularAutomaton, "cellular_automaton", make<CellularAutomatonStrategy>, false},
  {MazeAlgorithm::Ellers, "ellers", make<EllersStrategy>, true},
  {MazeAlgorithm::BinaryTree, "binary_tree", make<BinaryTreeStrategy>, true},
  {MazeAlgorithm::GrowingTree, "growing_tree", make<GrowingTreeStrategy>, true},
  {MazeAlgorithm::HuntAndKill, "hunt_and_kill", make<HuntAndKillStrategy>, true},
  {MazeAlgorithm::Kruskal, "kruskal", make<KruskalsStrategy>, true},
  {MazeAlgorithm::Prim, "prim", make<PrimsStrategy>, true},
  {MazeAlgorithm::RecursiveDivision, "recursive_division", make<RecursiveDivisionStrategy>, true},
  {MazeAlgorithm::Sidewinder, "sidewinder", make<SidewinderStrategy>, true},
  {MazeAlgorithm::Wilson, "wilson", make<WilsonsStrategy>, true},
  {MazeAlgorithm::Houston, "houston", make<HoustonStrategy>, true},
}};

const std::array<MazeAlgorithm, kAlgorithms.size()> kAlgorithmIds = [] {
    std::array<MazeAlgorithm, kAlgorithms.size()> ids{};
    for (size_t i = 0; i < kAlgorithms.size(); ++i) {
      ids[i] = kAlgorithms[i].id;
    }
    return ids;
  }();

}  // namespace

const AlgorithmInfo & algorithm_info(MazeAlgorithm algorithm)
{
  for (const AlgorithmInfo & info : kAlgorithms) {
    if (info.id == algorithm) {
      return info;
    }
  }
  throw std::invalid_argument(
          "maze_gen: unknown MazeAlgorithm value " + std::to_string(static_cast<int>(algorithm)));
}

std::span<const MazeAlgorithm> all_algorithms()
{
  return kAlgorithmIds;
}

std::string_view to_string(MazeAlgorithm algorithm)
{
  return algorithm_info(algorithm).name;
}

std::optional<MazeAlgorithm> algorithm_from_string(std::string_view name)
{
  for (const AlgorithmInfo & info : kAlgorithms) {
    if (info.name == name) {
      return info.id;
    }
  }
  return std::nullopt;
}

}  // namespace maze_gen
