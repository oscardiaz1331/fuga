#include "maze_gen/export.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <random>
#include <utility>

maze_gen::MazeExporter::MazeExporter(const int width, const int height,
                                     std::optional<uint64_t> seed)
    : generator_(), options_(), maze_(width, height) {
  if (!seed) {
    std::random_device rd;
    seed = (static_cast<uint64_t>(rd()) << 32) | rd();
  }
  options_.seed = seed;
}

maze_gen::MazeExporter::Point maze_gen::MazeExporter::cell_center(int x,
                                                                  int y) {
  return {(x + 0.5) * kCell, (y + 0.5) * kCell};
}

void maze_gen::MazeExporter::export_maze(const Outputs &out) {
  if (generate_)
    generator_.generate(maze_, options_);
  const auto walls = extract_walls(maze_);

  for (std::ostream *os : {out.sdf, out.pgm, out.map_yaml, out.meta}) {
    if (os)
      os->imbue(std::locale::classic());
  }
  if (out.sdf)
    write_sdf(*out.sdf, walls);
  if (out.pgm)
    write_pgm(*out.pgm, walls);
  if (out.map_yaml)
    write_map_yaml(*out.map_yaml, out.pgm_name);
  if (out.meta) {
    const Point start = cell_center(0, 0);
    const Point exit =
        maze_.exits().empty()
            ? start
            : cell_center(maze_.exits().front().x, maze_.exits().front().y);
    write_meta(*out.meta, start, exit);
  }
}

// Cada muro interior lo comparten dos celdas, asi que se mira una sola vez por
// linea de rejilla, y los muros contiguos de una misma linea se funden en una
// sola caja.
std::vector<maze_gen::MazeExporter::Wall>
maze_gen::MazeExporter::extract_walls(const maze_gen::MazeGrid &maze) const {
  const int W = maze.width(), Ht = maze.height();
  const auto horizontal = [&](int x,
                              int y) { // muro sobre la linea y, entre x y x+1
    return y < Ht ? maze.has_wall(x, y, maze_gen::Dir::N)
                  : maze.has_wall(x, Ht - 1, maze_gen::Dir::S);
  };
  const auto vertical = [&](int x,
                            int y) { // muro sobre la linea x, entre y e y+1
    return x < W ? maze.has_wall(x, y, maze_gen::Dir::W)
                 : maze.has_wall(W - 1, y, maze_gen::Dir::E);
  };

  std::vector<Wall> walls;
  for (int y = 0; y <= Ht; ++y) {
    for (int x = 0; x < W;) {
      if (!horizontal(x, y)) {
        ++x;
        continue;
      }
      const int start = x;
      while (x < W && horizontal(x, y))
        ++x;
      // + T para que las esquinas queden cerradas
      walls.push_back({(start + x) / 2.0 * kCell, y * double{kCell},
                       (x - start) * double{kCell} + T, 0.0});
    }
  }
  for (int x = 0; x <= W; ++x) {
    for (int y = 0; y < Ht;) {
      if (!vertical(x, y)) {
        ++y;
        continue;
      }
      const int start = y;
      while (y < Ht && vertical(x, y))
        ++y;
      walls.push_back({x * double{kCell}, (start + y) / 2.0 * kCell,
                       (y - start) * double{kCell} + T, M_PI / 2});
    }
  }
  return walls;
}

void maze_gen::MazeExporter::write_sdf(
    std::ostream &os,
    const std::vector<maze_gen::MazeExporter::Wall> &walls) const {
  os << "<?xml version=\"1.0\"?>\n<sdf version=\"1.9\"><world name=\"maze\">\n"
        "<plugin filename=\"gz-sim-physics-system\" "
        "name=\"gz::sim::systems::Physics\"/>\n"
        "<plugin filename=\"gz-sim-user-commands-system\" "
        "name=\"gz::sim::systems::UserCommands\"/>\n"
        "<plugin filename=\"gz-sim-scene-broadcaster-system\" "
        "name=\"gz::sim::systems::SceneBroadcaster\"/>\n"
        // Sensores del robot: Sensors genera los que necesitan render (lidar,
        // camaras) y Imu los de IMU; sin ellos los sensores no publican nada
        "<plugin filename=\"gz-sim-sensors-system\" "
        "name=\"gz::sim::systems::Sensors\">"
        "<render_engine>ogre2</render_engine></plugin>\n"
        "<plugin filename=\"gz-sim-imu-system\" "
        "name=\"gz::sim::systems::Imu\"/>\n"
        "<light type=\"directional\" name=\"sun\"><pose>0 0 10 0 0 0</pose>"
        "<direction>-0.5 0.1 -0.9</direction><diffuse>1 1 1 "
        "1</diffuse></light>\n";

  // Suelo: sin el, todo lo que no es static (el robot) cae al vacio. La
  // colision de un plane es infinita; size solo da el tamano visual, que cubre
  // el laberinto con 1 m de margen por cada lado
  const double gx = maze_.width() * kCell, gy = maze_.height() * kCell;
  os << "<model name=\"ground_plane\"><static>true</static>"
     << "<pose>" << gx / 2 << ' ' << gy / 2 << " 0 0 0 0</pose>"
     << "<link name=\"link\">";
  for (const char *kind : {"collision", "visual"}) {
    os << "<" << kind << " name=\"ground_" << kind << "\"><geometry><plane>"
       << "<normal>0 0 1</normal><size>" << gx + 2 << ' ' << gy + 2
       << "</size></plane></geometry>"
       << (kind[0] == 'v' ? "<material><ambient>0.8 0.8 0.8 1</ambient>"
                            "<diffuse>0.8 0.8 0.8 1</diffuse></material>"
                          : "")
       << "</" << kind << ">";
  }
  os << "</link></model>\n"
        "<model name=\"maze\"><static>true</static><link name=\"walls\">\n";

  int i = 0;
  for (const auto &w : walls) {
    for (const char *kind : {"collision", "visual"}) {
      os << "<" << kind << " name=\"w" << i << (kind[0] == 'c' ? "_c" : "_v")
         << "\">"
         << "<pose>" << w.x << ' ' << w.y << ' ' << H / 2 << " 0 0 " << w.yaw
         << "</pose>"
         << "<geometry><box><size>" << w.len << ' ' << T << ' ' << H
         << "</size></box></geometry>"
         << "</" << kind << ">\n";
    }
    ++i;
  }
  os << "</link></model></world></sdf>\n";
}

void maze_gen::MazeExporter::write_pgm(
    std::ostream &os,
    const std::vector<maze_gen::MazeExporter::Wall> &walls) const {
  const int w_px = maze_.width() * kPxPerCell;
  const int h_px = maze_.height() * kPxPerCell;
  const double px = 1.0 / kResolution; // pixeles por metro
  std::vector<uint8_t> image(static_cast<size_t>(w_px) * h_px, 255);

  const int thickness = std::max(1, static_cast<int>(std::lround(T * px)));
  // Eje del largo: de punta a punta, recortado a la imagen
  const auto along = [&](double center, double len, int n) {
    return std::pair{
        std::max(0, static_cast<int>(std::lround((center - len / 2) * px))),
        std::min(n, static_cast<int>(std::lround((center + len / 2) * px)))};
  };
  // Eje del grosor: siempre al menos 1 px, y los muros del borde no se salen de
  // la imagen
  const auto across = [&](double center, int n) {
    const int lo =
        std::clamp(static_cast<int>(std::lround(center * px - thickness / 2.0)),
                   0, n - thickness);
    return std::pair{lo, lo + thickness};
  };

  for (const auto &w : walls) {
    const bool vertical = w.yaw != 0.0;
    const auto [x0, x1] =
        vertical ? across(w.x, w_px) : along(w.x, w.len, w_px);
    const auto [y0, y1] =
        vertical ? along(w.y, w.len, h_px) : across(w.y, h_px);
    for (int y = y0; y < y1; ++y) {
      std::fill(image.begin() + static_cast<size_t>(y) * w_px + x0,
                image.begin() + static_cast<size_t>(y) * w_px + x1, 0);
    }
  }
  os << "P5\n" << w_px << ' ' << h_px << "\n255\n";
  // En el PGM la fila 0 es la de arriba, pero el origen del mapa es la esquina
  // de abajo: las filas se escriben de la ultima a la primera para que coincida
  // con el SDF
  for (int y = h_px - 1; y >= 0; --y) {
    os.write(reinterpret_cast<const char *>(image.data() +
                                            static_cast<size_t>(y) * w_px),
             w_px);
  }
}

void maze_gen::MazeExporter::write_map_yaml(std::ostream &os,
                                            const std::string &pgm_name) const {
  os << "image: " << pgm_name << "\nmode: trinary\nresolution: " << kResolution
     << "\norigin: [" << kOriginX << ", " << kOriginY << ", 0.0]"
     << "\nnegate: 0\noccupied_thresh: 0.65\nfree_thresh: 0.25\n";
}

void maze_gen::MazeExporter::write_meta(std::ostream &os, Point start,
                                        Point exit) const {
  if (options_.seed)
    os << "seed: " << *options_.seed << '\n';
  os << "cell: " << kCell << "\nstart: ["
     << start.x << ", " << start.y << "]"
     << "\nexit: [" << exit.x << ", " << exit.y << "]\n";
}

maze_gen::MazeExporterNode::MazeExporterNode() : Node("maze_exporter") {
  const int width = this->declare_parameter("width", 10);
  const int height = this->declare_parameter("height", 10);
  const int seed = this->declare_parameter("seed", int64_t{0});
  const auto now = std::chrono::system_clock::now();
  const std::string date = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
  const std::string date_str = "maze_" + date;
  const std::string folder = this->declare_parameter("folder", date_str);
  declare_parameter("sdf_file", folder+"/maze.sdf");
  declare_parameter("pgm_file", folder+"/maze.pgm");
  declare_parameter("map_yaml_file", folder+"/maze.yaml");
  declare_parameter("meta_file", folder+"/meta.yaml");
  exporter_ = std::make_unique<maze_gen::MazeExporter>(
      width, height,
      seed == 0 ? std::nullopt
                : std::make_optional(static_cast<uint64_t>(seed)));
  export_maze();
}

void maze_gen::MazeExporterNode::export_maze() {
  RCLCPP_INFO(this->get_logger(), "Exporting maze");
  const auto sdf_path = get_parameter("sdf_file").as_string();
  const auto pgm_path = get_parameter("pgm_file").as_string();
  const auto yaml_path = get_parameter("map_yaml_file").as_string();
  const auto meta_path = get_parameter("meta_file").as_string();

  std::ofstream sdf, pgm, yaml, meta;
  const auto open = [&](std::ofstream &f, const std::string &path,
                        auto mode) -> std::ostream * {
    if (path.empty())
      return nullptr;
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path());
    f.open(path, mode);
    if (!f) {
      RCLCPP_ERROR(get_logger(), "No se puede escribir en %s", path.c_str());
      return nullptr;
    }
    return &f;
  };

  maze_gen::MazeExporter::Outputs out;
  out.sdf = open(sdf, sdf_path, std::ios::out);
  out.pgm = open(pgm, pgm_path, std::ios::out | std::ios::binary);
  out.map_yaml = open(yaml, yaml_path, std::ios::out);
  out.meta = open(meta, meta_path, std::ios::out);
  out.pgm_name = pgm_path.empty() ? "" : std::filesystem::path(pgm_path).filename().string();

  exporter_->export_maze(out);

  RCLCPP_INFO(this->get_logger(), "Exported maze");
}