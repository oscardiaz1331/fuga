from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, IncludeLaunchDescription,
                            RegisterEventHandler, Shutdown)
from launch.conditions import IfCondition
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (LaunchConfiguration, PathJoinSubstitution,
                                  PythonExpression)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def include(package, launch_file, arguments):
    """IncludeLaunchDescription de <package>/launch/<launch_file>."""
    return IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare(package), 'launch', launch_file])),
        launch_arguments=arguments.items())


def is_create(action):
    """True para el nodo create de spawn.launch.py (mete el robot en Gazebo).

    Esta dentro de un include, asi que no hay variable con la que apuntarlo y
    se reconoce por el nombre del ejecutable (mismo truco que la camara de
    spawn.launch.py)
    """
    return getattr(action, 'node_executable', None) == 'create'


def generate_launch_description():
    # Argumentos de cada launch incluido, reexpuestos aqui con los mismos
    # nombres y valores por defecto para poder cambiarlos desde bringup
    maze_args = {'folder': '/tmp/maze', 'width': '10', 'height': '10',
                 'difficulty': '10', 'seed': '0', 'start_x': '0', 'start_y': '0'}
    spawn_args = {'rviz': 'true'}

    declare_args = [DeclareLaunchArgument(name, default_value=value)
                    for name, value in {**maze_args, **spawn_args}.items()]
    declare_slam = DeclareLaunchArgument(
        'slam', default_value='true',
        description='false para lanzar solo mundo y robot (p. ej. agentes N1)')
    declare_agent = DeclareLaunchArgument(
        'agent', default_value='wall_follower',
        description='wall_follower, pledge, o none para no lanzar agente ni '
                    'juez (p. ej. para mover el robot con teleop)')
    declare_results = DeclareLaunchArgument(
        'results',
        default_value=PathJoinSubstitution([LaunchConfiguration('folder'),
                                            'results.csv']),
        description='CSV donde el juez anade una fila por ejecucion')
    declare_maps = DeclareLaunchArgument(
        'maps',
        default_value=PathJoinSubstitution([LaunchConfiguration('folder'),
                                            'maps']),
        description='Carpeta donde el juez guarda el mapa SLAM al terminar '
                    '(con slam:=true) y su IoU frente a maze.pgm')
    agent = LaunchConfiguration('agent')

    # 1) Mundo: genera el laberinto y abre Gazebo con el
    maze = include('maze_export', 'maze.launch.py',
                   {name: LaunchConfiguration(name) for name in maze_args})

    # 2) Robot y puente. Puede arrancar a la vez que el mundo: el nodo create
    # repite la peticion a Gazebo hasta que este responde
    # La posicion sale de start_x/start_y, los mismos que recibe maze2world,
    # para que el robot aparezca en el start de meta.yaml: centro de la celda,
    # con celdas de 1 m (kCell en maze_export/export.hpp)
    spawn = include('fuga_description', 'spawn.launch.py',
                    {**{name: LaunchConfiguration(name) for name in spawn_args},
                     'x': PythonExpression([LaunchConfiguration('start_x'), ' + 0.5']),
                     'y': PythonExpression([LaunchConfiguration('start_y'), ' + 0.5'])})

    # 3) SLAM, cuando el robot ya existe (sale el nodo create). Si arrancara
    # antes no habria TF odom -> base_link ni /scan y daria avisos al principio
    slam = RegisterEventHandler(
        OnProcessExit(
            target_action=is_create,
            on_exit=[include('fuga_slam', 'slam.launch.py', {})]),
        condition=IfCondition(LaunchConfiguration('slam')))

    # 4) Agente y juez, juntos y tambien cuando el robot ya existe. Juntos para
    # que el start_timeout del juez no corra mientras el agente aun no esta.
    # El nombre del agente va a los dos: al agente para elegir cual crear y al
    # juez para apuntarlo en el CSV
    agent_node = Node(
        package='fuga_agents', executable='fuga_agents',
        arguments=[agent],
        # use_sim_time: el agente usa now() (exit_timeout)
        parameters=[{'use_sim_time': True}],
        output='screen')
    judge = Node(
        package='fuga_eval', executable='judge_node',
        parameters=[{
            'meta_file': PathJoinSubstitution([LaunchConfiguration('folder'),
                                               'meta.yaml']),
            'csv_file': LaunchConfiguration('results'),
            'algorithm': agent,
            # Sin SLAM no hay /map que guardar
            'save_map': LaunchConfiguration('slam'),
            'map_dir': LaunchConfiguration('maps'),
        }],
        output='screen')
    run = RegisterEventHandler(
        OnProcessExit(target_action=is_create, on_exit=[agent_node, judge]),
        condition=IfCondition(PythonExpression(["'", agent, "' != 'none'"])))

    # 5) Cuando el juez termina (exit, timeout o no_start, ya con la fila en el
    # CSV) se cierra todo: Gazebo, agente, SLAM, RViz. Asi cada ros2 launch
    # vuelve solo y un script puede encadenar ejecuciones
    stop_all = RegisterEventHandler(
        OnProcessExit(target_action=judge,
                      on_exit=[Shutdown(reason='el juez ha terminado')]))

    return LaunchDescription([
        *declare_args,
        declare_slam,
        declare_agent,
        declare_results,
        declare_maps,
        maze,
        spawn,
        slam,
        run,
        stop_all,
    ])
