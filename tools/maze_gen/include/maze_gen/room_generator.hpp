#pragma once
#include <random>
#include "maze_gen/room.hpp"
#include "maze_gen/maze_grid.hpp"
#include "maze_gen/maze_strategy_options.hpp"

namespace maze_gen {

class RoomGenerator
{
private:
    std::mt19937 _rng;
    // Configuration based on difficulty
    struct RoomConfig {
        int minRoomSize;
        int maxRoomSize;
        int minRooms;
        int maxRooms;
        double roomDensity;  // Percentage of maze that can be rooms
    };
    int _mazeWidth;
    int _mazeHeight;
    RoomConfig getRoomConfig(int diff, int w, int h) const;

    bool isValidRoomPlacement(const Room& room, const std::vector<Room>& existingRooms) const;

    Room generateRandomRoom(const RoomConfig& config);

public:
	RoomGenerator() : _rng(std::random_device{}()), _mazeWidth(0), _mazeHeight(0){}
    // Apply rooms to maze grid by clearing walls
    void generateRooms(MazeGrid& maze, const MazeStrategyOptions& opts);

private:
    std::vector<Room> createRooms(int mazeWidth, int mazeHeight, int difficulty);
    void clearRoomWalls(MazeGrid& maze, const Room& room);
    void createRoomConnections(MazeGrid& maze, const Room& room);
};

}  // namespace maze_gen
