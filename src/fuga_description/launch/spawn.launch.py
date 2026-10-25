from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, ExecuteProcess,
                            IncludeLaunchDescription, RegisterEventHandler)
from launch.conditions import IfCondition
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    x = LaunchConfiguration('x')
    y = LaunchConfiguration('y')

    # Por defecto, centro de la celda (0, 0) del laberinto (celdas de 1 m).
    # bringup.launch.py los calcula a partir de start_x/start_y
    declare_x = DeclareLaunchArgument('x', default_value='0.5')
    declare_y = DeclareLaunchArgument('y', default_value='0.5')
    declare_rviz = DeclareLaunchArgument('rviz', default_value='true')

    # Expande el xacro a URDF al lanzar, asi siempre se usa la version instalada
    robot_description = ParameterValue(
        Command(['xacro ', PathJoinSubstitution(
            [FindPackageShare('fuga_description'), 'urdf', 'fuga.urdf.xacro'])]),
        value_type=str)

    # Publica en TF los links del robot: los fixed desde el URDF y las ruedas a
    # partir de /joint_states. use_sim_time porque los mensajes llegan con la
    # hora de Gazebo (/clock); sin el los descartaria por antiguos
    robot_state_publisher = Node(
        package='robot_state_publisher', executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description,
                     'use_sim_time': True}],
        output='screen')

    # Launch de ros_gz_sim que hace dos cosas: crea el robot en el Gazebo que ya
    # este corriendo (p. ej. maze.launch.py), leyendo el modelo de
    # /robot_description, y arranca el puente Gazebo <-> ROS con los topics,
    # tipos y sentidos del YAML. z = wheel_r + 1 cm para que caiga sobre las
    # ruedas sin atravesar el suelo
    spawn_and_bridge = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare('ros_gz_sim'), 'launch',
             'ros_gz_spawn_model.launch.py'])),
        launch_arguments={
            'entity_name': 'fuga',
            'topic': 'robot_description',
            'x': x, 'y': y, 'z': '0.05',
            'bridge_name': 'fuga_bridge',
            'config_file': PathJoinSubstitution(
                [FindPackageShare('fuga_description'), 'config', 'bridge.yaml']),
        }.items())

    # Cuando el robot ya existe, pide a la GUI de Gazebo que la camara lo siga
    # (plugin CameraTracking, cargado por defecto). El offset es la posicion de
    # la camara respecto al robot: detras y por encima para verlo desde arriba.
    # El nodo create esta dentro del include y no se puede referenciar, asi que
    # se reconoce por el nombre del ejecutable
    follow_camera = RegisterEventHandler(OnProcessExit(
        target_action=lambda action:
            getattr(action, 'node_executable', None) == 'create',
        on_exit=[
            ExecuteProcess(cmd=[
                'gz', 'service', '-s', '/gui/follow/offset',
                '--reqtype', 'gz.msgs.Vector3d', '--reptype', 'gz.msgs.Boolean',
                '--timeout', '5000', '--req', 'x: -1.5, y: 0, z: 2'],
                output='screen'),
            ExecuteProcess(cmd=[
                'gz', 'service', '-s', '/gui/follow',
                '--reqtype', 'gz.msgs.StringMsg', '--reptype', 'gz.msgs.Boolean',
                '--timeout', '5000', '--req', 'data: "fuga"'],
                output='screen'),
        ]))

    # RViz con RobotModel, TF y LaserScan. use_sim_time para que acepte los
    # mensajes con la hora de Gazebo; sin el descarta TF y scan por antiguos
    rviz = Node(
        package='rviz2', executable='rviz2',
        arguments=['-d', PathJoinSubstitution(
            [FindPackageShare('fuga_description'), 'config', 'fuga.rviz'])],
        parameters=[{'use_sim_time': True}],
        condition=IfCondition(LaunchConfiguration('rviz')),
        output='screen')

    return LaunchDescription([
        declare_x,
        declare_y,
        declare_rviz,
        robot_state_publisher,
        follow_camera,
        spawn_and_bridge,
        rviz,
    ])
