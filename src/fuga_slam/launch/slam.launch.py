from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Por defecto, el YAML de este paquete (base_frame: base_link, rango del
    # lidar a 10 m). Se puede cambiar para probar otros parametros sin recompilar
    declare_params = DeclareLaunchArgument(
        'slam_params_file',
        default_value=PathJoinSubstitution(
            [FindPackageShare('fuga_slam'), 'config',
             'mapper_params_online_async.yaml']),
        description='YAML de parametros de slam_toolbox')

    # slam_toolbox en modo online asincrono: procesa el scan mas reciente y
    # descarta los que lleguen mientras esta ocupado, asi no se queda atrasado.
    # use_sim_time porque scan y TF llevan la hora de Gazebo (/clock)
    slam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare('slam_toolbox'), 'launch',
             'online_async_launch.py'])),
        launch_arguments={
            'slam_params_file': LaunchConfiguration('slam_params_file'),
            'use_sim_time': 'true',
        }.items())

    return LaunchDescription([
        declare_params,
        slam,
    ])
