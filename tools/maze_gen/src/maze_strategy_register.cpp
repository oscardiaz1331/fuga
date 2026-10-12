// register_strategies.cpp
#include "maze_gen/maze_strategy_factory.hpp"
#include "maze_gen/maze_algorithm.hpp"

#include "maze_gen/dfs_maze_strategy.hpp"
#include "maze_gen/aldous_broder_strategy.hpp"
#include "maze_gen/cellular_automaton_strategy.hpp"
#include "maze_gen/ellers_strategy.hpp"
#include "maze_gen/growing_tree_strategy.hpp"
#include "maze_gen/binary_tree_strategy.hpp"
#include "maze_gen/hunt_and_kill_strategy.hpp"
#include "maze_gen/kruskals_strategy.hpp"
#include "maze_gen/prims_strategy.hpp"
#include "maze_gen/recursive_division_strategy.hpp"
#include "maze_gen/sidewinder_strategy.hpp"
#include "maze_gen/wilsons_strategy.hpp"

#include <memory>
#include <string>
#include "maze_gen/maze_strategy_register.hpp"

namespace maze_gen {

void MazeStrategyRegister::registerAllStrategies() {
    using SF = MazeStrategyFactory;
    using Creator = MazeStrategyFactory::Creator;

    // Helper to register both enum and string name
    auto reg = [&](MazeAlgorithm key, Creator creator) {
        SF::instance().registerCreator(key, creator);
        };

    // DFS
    reg(MazeAlgorithm::DFS, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<DFSMazeStrategy>();
        });

    // Aldous-Broder
    reg(MazeAlgorithm::AldousBroder, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<AldousBroderStrategy>();
        });

    // Cellular Automaton (good for caves / open maps)
    reg(MazeAlgorithm::CellularAutomaton, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<CellularAutomatonStrategy>();
        });

    // Eller's
    reg(MazeAlgorithm::Ellers, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<EllersStrategy>();
        });

    // Growing Tree
    reg(MazeAlgorithm::GrowingTree, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<GrowingTreeStrategy>();
        });

    // Growing Tree
    reg(MazeAlgorithm::BinaryTree, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<BinaryTreeStrategy>();
        });

    // Hunt-and-Kill
    reg(MazeAlgorithm::HuntAndKill, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<HuntAndKillStrategy>();
        });

    // Kruskal
    reg(MazeAlgorithm::Kruskal, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<KruskalsStrategy>();
        });

    // Prim
    reg(MazeAlgorithm::Prim, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<PrimsStrategy>();;
        });

    // Recursive Division
    reg(MazeAlgorithm::RecursiveDivision, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<RecursiveDivisionStrategy>();
        });

    // Sidewinder
    reg(MazeAlgorithm::Sidewinder, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<SidewinderStrategy>();
        });

    // Wilson
    reg(MazeAlgorithm::Wilson, []() -> std::unique_ptr<IMazeGenerationStrategy> {
        return std::make_unique<WilsonsStrategy>();
        });
}

}  // namespace maze_gen
