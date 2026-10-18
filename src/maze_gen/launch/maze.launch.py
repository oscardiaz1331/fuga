from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, ExecuteProcess,
                            IncludeLaunchDescription, RegisterEventHandler)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    folder = LaunchConfiguration('folder')

    declare_folder = DeclareLaunchArgument(
        'folder', default_value='/tmp/maze',
        description='Carpeta donde maze2world escribe maze.sdf, maze.pgm, ...')

    # 1) Ejecuta maze2world con esa carpeta (genera el mundo y termina)
    generate_world = ExecuteProcess(
        cmd=['ros2', 'run', 'maze_export', 'maze2world',
             '--ros-args', '-p', ['folder:=', folder]],
        output='screen')

    # 2) Cuando termina, lanza Gazebo con el mundo de esa carpeta
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([FindPackageShare('ros_gz_sim'),
                                  'launch', 'gz_sim.launch.py'])),
        launch_arguments={
            'gz_args': ['-r -v 3 ', PathJoinSubstitution([folder, 'maze.sdf'])]
        }.items())

    start_sim_after_generation = RegisterEventHandler(
        OnProcessExit(target_action=generate_world, on_exit=[gz_sim]))

    return LaunchDescription([
        declare_folder,
        generate_world,
        start_sim_after_generation,
    ])
