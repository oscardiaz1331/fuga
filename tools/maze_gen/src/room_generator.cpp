#include "maze_gen/room_generator.hpp"
#include <algorithm>
#include "maze_gen/random_generator.hpp"

namespace maze_gen {

RoomGenerator::RoomConfig RoomGenerator::getRoomConfig(int diff, int w, int h) const
{
    // Clamp difficulty
    diff = std::max(0, std::min(10, diff));
    // Base calculations
    int totalCells = w * h;
    int minDimension = std::min(w, h);
    int maxDimension = std::max(w, h);

    RoomConfig config;

    // Room sizes scale with difficulty and maze size
    config.minRoomSize = std::max(2, minDimension / 15 + diff / 3);
    config.maxRoomSize = std::max(config.minRoomSize + 1,
        minDimension / 8 + diff / 2);

    // Number of rooms
    double baseDensity = 0.02 + (diff * 0.008);  // 2% to 10%
    int baseRooms = static_cast<int>(totalCells * baseDensity / 20);

    config.minRooms = std::max(1, baseRooms);
    config.maxRooms = std::max(config.minRooms, baseRooms + diff / 2);

    // Room density (how much of the maze can be rooms)
    config.roomDensity = std::min(0.25, 0.05 + diff * 0.02);

    return config;
}

bool RoomGenerator::isValidRoomPlacement(const Room& room, const std::vector<Room>& existingRooms) const
{
    // Check boundaries
	auto [x, y] = room.topLeft();
    if (x < 1 || y < 1 ||
        x + room.width() >= _mazeWidth - 1 ||
        y + room.height() >= _mazeHeight - 1) {
        return false;
    }

    // Check overlaps with existing rooms
    for (const auto& existing : existingRooms) {
        if (room.overlaps(existing)) {
            return false;
        }
    }

    return true;
}

Room RoomGenerator::generateRandomRoom(const RoomConfig& config)
{
    std::uniform_int_distribution<int> sizeRand(config.minRoomSize, config.maxRoomSize);
    std::uniform_int_distribution<int> xRand(1, _mazeWidth - config.maxRoomSize - 1);
    std::uniform_int_distribution<int> yRand(1, _mazeHeight - config.maxRoomSize - 1);

    int width = sizeRand(_rng);
    int height = sizeRand(_rng);
    int x = xRand(_rng);
    int y = yRand(_rng);

    // Ensure room fits
    x = std::min(x, _mazeWidth - width - 1);
    y = std::min(y, _mazeHeight - height - 1);

    return Room(x, y, width, height);
}

std::vector<Room> RoomGenerator::createRooms(int mazeWidth, int mazeHeight, int difficulty)
{
    std::vector<Room> rooms;
    RoomConfig config = getRoomConfig(difficulty, mazeWidth, mazeHeight);
    _mazeWidth = mazeWidth;
    _mazeHeight = mazeHeight;
    std::uniform_int_distribution<int> roomCountRand(config.minRooms, config.maxRooms);
    int targetRooms = roomCountRand(_rng);

    // Calculate maximum allowed room area
    int totalCells = mazeWidth * mazeHeight;
    int maxRoomArea = static_cast<int>(totalCells * config.roomDensity);
    int currentRoomArea = 0;

    // Try to place rooms
    int attempts = 0;
    const int maxAttempts = targetRooms * 50; // Prevent infinite loops

    while (rooms.size() < static_cast<size_t>(targetRooms) && attempts < maxAttempts) {
        Room candidateRoom = generateRandomRoom(config);
        int roomArea = candidateRoom.width() * candidateRoom.height();

        if (currentRoomArea + roomArea <= maxRoomArea &&
            isValidRoomPlacement(candidateRoom, rooms)) {
            rooms.push_back(candidateRoom);
            currentRoomArea += roomArea;
        }

        attempts++;
    }

    return rooms;
}

void RoomGenerator::generateRooms(MazeGrid& grid, const MazeStrategyOptions& opts)
{
    _rng = RandomGenerator::generateRng(opts.seed);
	auto rooms = createRooms(grid.width(), grid.height(), opts.difficulty);
    for (const auto& room : rooms) {
        clearRoomWalls(grid, room);
    }

    // Connect rooms to maze passages
    for (const auto& room : rooms) {
        createRoomConnections(grid, room);
    }
}

void RoomGenerator::clearRoomWalls(MazeGrid& grid, const Room& room)
{
    // Clear all internal walls in the room
    auto [x, y] = room.topLeft();
    for (int ry = y; ry < y + room.height(); ++ry) {
        for (int rx = x; rx < x + room.width(); ++rx) {
            if (!grid.in_bounds(rx, ry)) continue;

            // Clear internal connections
            if (rx < x + room.width() - 1) {
                // Clear east wall if not at room's right edge
				grid.carve(rx, ry, Dir::E);
            }
            if (ry < y + room.height() - 1) {
                // Clear south wall if not at room's bottom edge
                grid.carve(rx, ry, Dir::S);
            }
            if (rx > x) {
                // Clear west wall if not at room's left edge
                grid.carve(rx, ry, Dir::W);
            }
            if (ry > y) {
                // Clear north wall if not at room's top edge
                grid.carve(rx, ry, Dir::N);
            }
        }
    }
}

void RoomGenerator::createRoomConnections(MazeGrid& grid, const Room& room)
{

    std::vector<std::pair<int, int>> perimeter = room.getPerimeterCells();

    // Shuffle perimeter to randomize connection points
    std::shuffle(perimeter.begin(), perimeter.end(), _rng);

    // Create 1-3 connections based on room size
    int connections = std::min(3, std::max(1, (room.width() + room.height()) / 8));

    int made_connections = 0;
    for (const auto& [px, py] : perimeter) {
        if (made_connections >= connections) break;

        // Try to connect to adjacent maze passages
        auto directions = grid.directions4();
        std::shuffle(directions.begin(), directions.end(), _rng);

        for (auto dir : directions) {
            auto neighbor = grid.try_neighbor(px, py, dir);
            if (neighbor && !room.contains(neighbor->first, neighbor->second)) {
                // Connect to this neighbor
                if (grid.carve(px, py, dir)) {
                    made_connections++;
                    break;
                }
            }
        }
    }
}

}  // namespace maze_gen
