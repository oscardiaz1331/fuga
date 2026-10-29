from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Por defecto, el YAML de este paquete (radio del robot, alturas del
    # collision_monitor). Se puede cambiar para probar otros sin recompilar
    declare_params = DeclareLaunchArgument(
        'params_file',
        default_value=PathJoinSubstitution(
            [FindPackageShare('fuga_nav'), 'config', 'nav2_params.yaml']),
        description='YAML de parametros de Nav2')

    # navigation_launch.py y no bringup_launch.py: este ultimo anade map_server
    # y AMCL, que chocarian con SLAM (ya publica /map y map -> odom).
    # use_sim_time porque scan y TF llevan la hora de Gazebo (/clock)
    nav2 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare('nav2_bringup'), 'launch',
             'navigation_launch.py'])),
        launch_arguments={
            'params_file': LaunchConfiguration('params_file'),
            'use_sim_time': 'true',
        }.items())

    return LaunchDescription([
        declare_params,
        nav2,
    ])
