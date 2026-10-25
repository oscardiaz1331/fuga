from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, ExecuteProcess,
                            IncludeLaunchDescription, RegisterEventHandler,
                            Shutdown)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    folder = LaunchConfiguration('folder')
    width = LaunchConfiguration('width')
    height = LaunchConfiguration('height')
    difficulty = LaunchConfiguration('difficulty')
    seed = LaunchConfiguration('seed')
    start_x = LaunchConfiguration('start_x')
    start_y = LaunchConfiguration('start_y')

    declare_args = [
        DeclareLaunchArgument(
            'folder', default_value='/tmp/maze',
            description='Carpeta donde maze2world escribe maze.sdf, maze.pgm, ...'),
        DeclareLaunchArgument('width', default_value='10',
                              description='Ancho del laberinto, en celdas'),
        DeclareLaunchArgument('height', default_value='10',
                              description='Alto del laberinto, en celdas'),
        DeclareLaunchArgument(
            'difficulty', default_value='10',
            description='0 a 10. Con 10 es perfecto; cuanto mas bajo, mas bucles e islas'),
        DeclareLaunchArgument(
            'seed', default_value='0',
            description='0 = al azar. Con la seed de meta.yaml se repite el mismo '
                        'laberinto, para comparar algoritmos en igualdad'),
        DeclareLaunchArgument(
            'start_x', default_value='0',
            description='Columna de la celda donde aparece el robot. meta.yaml '
                        'mide desde ella las distancias a las salidas'),
        DeclareLaunchArgument('start_y', default_value='0',
                              description='Fila de la celda donde aparece el robot'),
    ]

    # 1) Ejecuta maze2world con esos parametros (genera el mundo y termina)
    generate_world = ExecuteProcess(
        cmd=['ros2', 'run', 'maze_export', 'maze2world', '--ros-args',
             '-p', ['folder:=', folder],
             '-p', ['width:=', width],
             '-p', ['height:=', height],
             '-p', ['difficulty:=', difficulty],
             '-p', ['seed:=', seed],
             '-p', ['start_x:=', start_x],
             '-p', ['start_y:=', start_y]],
        output='screen')

    # 2) Cuando termina, lanza Gazebo con el mundo de esa carpeta
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([FindPackageShare('ros_gz_sim'),
                                  'launch', 'gz_sim.launch.py'])),
        launch_arguments={
            'gz_args': ['-r -v 3 ', PathJoinSubstitution([folder, 'maze.sdf'])]
        }.items())

    # Si maze2world falla (p. ej. start fuera del laberinto) se para todo, en
    # vez de abrir Gazebo con el maze.sdf que hubiera de antes
    def on_generated(event, context):
        if event.returncode != 0:
            return Shutdown(reason='maze2world ha fallado (ver el error de arriba)')
        return gz_sim

    start_sim_after_generation = RegisterEventHandler(
        OnProcessExit(target_action=generate_world, on_exit=on_generated))

    return LaunchDescription([
        *declare_args,
        generate_world,
        start_sim_after_generation,
    ])
