// Demo: writes one text file per algorithm with the same maze at three difficulties.
// Usage: maze_gen_demo [output_directory]   (default: ./data)
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "maze_gen/maze_generator.hpp"

int main(int argc, char ** argv)
{
  const std::filesystem::path output_dir = argc > 1 ? argv[1] : "data";
  std::error_code error;
  std::filesystem::create_directories(output_dir, error);
  if (error) {
    std::cerr << "Error: could not create " << output_dir << ": " << error.message() << "\n";
    return 1;
  }

  maze_gen::MazeStrategyOptions options;
  options.seed = 123456789;
  options.num_random_exits = 10;
  maze_gen::MazeGrid maze(50, 50);

  int failures = 0;
  for (const maze_gen::MazeAlgorithm algorithm : maze_gen::all_algorithms()) {
    const std::string name(maze_gen::to_string(algorithm));
    const std::filesystem::path file = output_dir / (name + ".txt");
    std::ofstream out(file, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
      std::cerr << "Error: could not open " << file << " for writing\n";
      ++failures;
      continue;
    }

    maze_gen::MazeGenerator generator{algorithm};
    for (const int difficulty : {10, 5, 0}) {
      options.difficulty = difficulty;
      const auto start = std::chrono::steady_clock::now();
      generator.generate(maze, options);
      const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - start;

      out << "Maze with difficulty " << difficulty << ":\nSimple:\n";
      maze.display_simple(out);
      out << "\nComplex:\n";
      maze.display_complex(out);
      out << "\nMaze generated in " << elapsed.count() << " ms\n\n";
    }
    std::cout << "Maze " << name << " saved to " << file.string() << "\n";
  }
  return failures == 0 ? 0 : 1;
}
