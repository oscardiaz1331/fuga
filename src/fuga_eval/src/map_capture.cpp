#include "fuga_eval/map_capture.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <sstream>

std::string fuga_eval::file_stamp(std::time_t t)
{
  std::tm local{};
  localtime_r(&t, &local);
  std::ostringstream os;
  os << std::put_time(&local, "%Y-%m-%dT%H-%M-%S");
  return os.str();
}

std::string fuga_eval::shell_quote(std::string_view s)
{
  // Dentro de '...' el shell no interpreta nada; una ' se cierra, se escapa y
  // se vuelve a abrir: ' -> '\''
  std::string quoted = "'";
  for (const char c : s) {
    if (c == '\'') {
      quoted += "'\\''";
    } else {
      quoted += c;
    }
  }
  quoted += '\'';
  return quoted;
}

fuga_eval::MapCapture fuga_eval::make_map_capture(
  const std::string & dir,
  std::string name,
  const std::string & meta_file,
  double tolerance)
{
  namespace fs = std::filesystem;
  std::replace(name.begin(), name.end(), '/', '_');
  const std::string truth = (fs::path(meta_file).parent_path() / "maze.yaml").string();

  MapCapture c;
  c.prefix = (fs::path(dir) / name).string();
  c.map_yaml = c.prefix + ".yaml";
  c.diff_png = c.prefix + "_diff.png";
  c.iou_json = c.prefix + "_iou.json";

  // use_sim_time: /map lleva la hora de Gazebo, como todo lo demas
  c.save_command = "ros2 run nav2_map_server map_saver_cli -f " +
    shell_quote(c.prefix) +
    " --ros-args -p use_sim_time:=true";

  std::ostringstream tol;
  tol.imbue(std::locale::classic()); // 0.05, no 0,05
  tol << tolerance;
  c.iou_command = "ros2 run fuga_slam map_iou.py --slam " +
    shell_quote(c.map_yaml) + " --truth " + shell_quote(truth) +
    " --meta " + shell_quote(meta_file) + " --tolerance " +
    tol.str() + " --diff " + shell_quote(c.diff_png) + " --json";
  return c;
}
