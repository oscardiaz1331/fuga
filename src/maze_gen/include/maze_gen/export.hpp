#pragma once
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include "maze_gen/maze_generator.hpp"

namespace maze_gen
{
    class MazeExporter
    {
        public:
            // Streams where each file is written; the ones left null are skipped
            struct Outputs
            {
                std::ostream* sdf = nullptr;       // world.sdf para Gazebo
                std::ostream* pgm = nullptr;       // mapa de ocupacion
                std::ostream* map_yaml = nullptr;  // maze.yaml (formato nav2_map_server)
                std::ostream* meta = nullptr;      // meta.yaml (semilla, inicio y salida)
                std::string pgm_name = "maze.pgm"; // como lo referencia map_yaml, relativo a el
            };

            // Sin semilla se elige una al azar, y queda registrada en meta.yaml.
            // difficulty de 0 a 10: con 10 el laberinto es perfecto; por debajo hay bucles e islas
            MazeExporter(const int width, const int height, std::optional<uint64_t> seed = std::nullopt,
                         int difficulty = 10);
            // Exporta un laberinto ya hecho (p. ej. cargado de fichero): no se genera ni hay semilla
            explicit MazeExporter(maze_gen::MazeGrid grid) : maze_(std::move(grid)), generate_(false) {}
            void export_maze(const Outputs& out);
        private:
        struct Wall { double x, y, len, yaw; };// center, length, heading (rad)
        struct Point { double x, y; };
        maze_gen::MazeGenerator generator_;
        maze_gen::MazeStrategyOptions options_;
        maze_gen::MazeGrid maze_;
        bool generate_ = true;
          static constexpr float T = 0.1, H = 0.5, kCell = 1.0;  // thickness, height (m), cell size (m)
          static constexpr int kPxPerCell = 20;  // con 20: 0.05 m/px y los muros de 10 cm ocupan 2 px
          static constexpr double kResolution = kCell / kPxPerCell;  // m/px, compartido por pgm y maze.yaml
          // Esquina inferior izquierda del mapa en el mundo: el laberinto ocupa x en [0, W], y en [0, H]
          static constexpr double kOriginX = 0.0, kOriginY = 0.0;

        std::vector<maze_gen::MazeExporter::Wall> extract_walls(const maze_gen::MazeGrid& maze) const;
        // Celda donde aparece el robot (spawn.launch.py usa su centro por defecto)
        static constexpr int kStartX = 0, kStartY = 0;

        // Centro de una celda en coordenadas de mundo (m)
        static Point cell_center(int x, int y);
        // Centro del hueco que abre una salida en el muro exterior (m)
        static Point door(const maze_gen::Exit& exit);
        // BFS: pasos desde (x, y) hasta cada celda, en el orden de cell_index.
        // -1 en las celdas a las que no se puede llegar
        static std::vector<int> distances_from(const maze_gen::MazeGrid& maze, int x, int y);
        void write_sdf(std::ostream& os, const std::vector<maze_gen::MazeExporter::Wall>& walls) const;
        void write_pgm(std::ostream& os, const std::vector<maze_gen::MazeExporter::Wall>& walls) const;
        void write_map_yaml(std::ostream& os, const std::string& pgm_name) const;
        void write_meta(std::ostream& os) const;
    };
    
    class MazeExporterNode : public rclcpp::Node
    {
        public:
            MazeExporterNode();
        private:
            void export_maze();
           std::unique_ptr<maze_gen::MazeExporter> exporter_;
    };
} // namespace maze_gen
