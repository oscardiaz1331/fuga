#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include "maze_gen/maze_generator.hpp"

using maze_gen::MazeAlgorithm;
using maze_gen::MazeGenerator;
using maze_gen::MazeGrid;
using maze_gen::MazeStrategyOptions;

std::string algorithmName(MazeAlgorithm algo) {
	switch (algo) {
	case MazeAlgorithm::DFS: return "DFS";
	case MazeAlgorithm::AldousBroder: return "AldousBroder";
	case MazeAlgorithm::CellularAutomaton: return "CellularAutomaton";
	case MazeAlgorithm::Ellers: return "Ellers";
	case MazeAlgorithm::GrowingTree: return "GrowingTree";
	case MazeAlgorithm::BinaryTree: return "BinaryTree";
	case MazeAlgorithm::HuntAndKill: return "HuntAndKill";
	case MazeAlgorithm::Kruskal: return "Kruskal";
	case MazeAlgorithm::Prim: return "Prim";
	case MazeAlgorithm::RecursiveDivision: return "RecursiveDivision";
	case MazeAlgorithm::Sidewinder: return "Sidewinder";
	case MazeAlgorithm::Wilson: return "Wilson";
	default: return "Unknown";
	}
}

int main()
{
	MazeGenerator generator{};
	uint64_t seed = 123456789;
	MazeStrategyOptions opts;
	opts.difficulty = 2;
	opts.num_random_exits = 10; 
	opts.seed = seed;
	auto maze = MazeGrid(50, 50);
	std::vector<MazeAlgorithm> algorithms = {
		MazeAlgorithm::DFS,
		MazeAlgorithm::AldousBroder,
		MazeAlgorithm::CellularAutomaton,
		MazeAlgorithm::Ellers,
		MazeAlgorithm::BinaryTree,
		MazeAlgorithm::GrowingTree,
		MazeAlgorithm::HuntAndKill,
		MazeAlgorithm::Kruskal,
		MazeAlgorithm::Prim,
		MazeAlgorithm::RecursiveDivision,
		MazeAlgorithm::Sidewinder,
		MazeAlgorithm::Wilson
	};

	for (const auto algo : algorithms) {
		std::string filename = "data/" + algorithmName(algo) + ".txt";
		std::ofstream out(filename, std::ios::out | std::ios::binary);
		generator.setMazeStrategy(algo);
		if (out.is_open()) {
			for (int i = 0; i < 1; ++i) {
				opts.seed = seed + i;
				opts.difficulty = i;
				out << "Generating maze with difficulty " << i << ":\nSimple:\n";
				auto start = std::chrono::high_resolution_clock::now();
				generator.generate(maze, opts);
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> elapsed = end - start;
	
				maze.displaySimple(out);
				out << "\n Complex:\n";
				maze.displayComplex(out);
				out << "\nMaze generated in " << elapsed.count() << " ms\n";
			}
			std::cout << "\nMaze " + algorithmName(algo) + " saved\n";
			out.close();
		}
		else {
			std::cerr << "Error: Could not open file for writing.\n";
		}
	}
}
