#include <gtest/gtest.h>

#include <string>

#include "fuga_eval/map_capture.hpp"

using fuga_eval::make_map_capture;
using fuga_eval::shell_quote;

TEST(ShellQuote, EntreComillasSimples) {
  EXPECT_EQ(shell_quote("/tmp/maze/maps"), "'/tmp/maze/maps'");
  EXPECT_EQ(shell_quote("a b$c"), "'a b$c'");
}

TEST(ShellQuote, EscapaLasComillasDeDentro) {
  EXPECT_EQ(shell_quote("it's"), "'it'\\''s'");
}

TEST(MapCapture, RutasJuntoAlPrefijo) {
  const auto c = make_map_capture("/tmp/maze/maps", "2026-10-25T17-03-02_pledge",
                                  "/tmp/maze/meta.yaml", 0.05);
  EXPECT_EQ(c.prefix, "/tmp/maze/maps/2026-10-25T17-03-02_pledge");
  EXPECT_EQ(c.map_yaml, c.prefix + ".yaml");
  EXPECT_EQ(c.diff_png, c.prefix + "_diff.png");
  EXPECT_EQ(c.iou_json, c.prefix + "_iou.json");
}

TEST(MapCapture, ComandosConMapSaverYMapIou) {
  const auto c = make_map_capture("/tmp/maze/maps", "run", "/tmp/maze/meta.yaml",
                                  0.05);
  EXPECT_EQ(c.save_command,
            "ros2 run nav2_map_server map_saver_cli -f '/tmp/maze/maps/run' "
            "--ros-args -p use_sim_time:=true");
  EXPECT_EQ(c.iou_command,
            "ros2 run fuga_slam map_iou.py --slam '/tmp/maze/maps/run.yaml' "
            "--truth '/tmp/maze/maze.yaml' --meta '/tmp/maze/meta.yaml' "
            "--tolerance 0.05 --diff '/tmp/maze/maps/run_diff.png' --json");
}

TEST(MapCapture, NombreSinBarras) {
  const auto c = make_map_capture("/m", "a/b", "/tmp/maze/meta.yaml", 0.0);
  EXPECT_EQ(c.prefix, "/m/a_b");
}
