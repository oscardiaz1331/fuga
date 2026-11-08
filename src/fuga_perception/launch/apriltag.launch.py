from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import (EqualsSubstitution, LaunchConfiguration,
                                  PathJoinSubstitution)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    detector = LaunchConfiguration('detector')
    # Los dos detectores leen el mismo yaml: size y el resto de parametros en un solo sitio
    params = [PathJoinSubstitution(
        [FindPackageShare('fuga_perception'), 'config', 'apriltag.yaml']),
        {'use_sim_time': True}]

    # Solo se lanza uno: los dos publican la TF exit_tag y no deben coincidir
    declare_detector = DeclareLaunchArgument(
        'detector', default_value='apriltag_ros',
        choices=['apriltag_ros', 'opencv'],
        description='apriltag_ros (paquete de ROS) u opencv (AprilTagFinder de '
                    'fuga_perception)')

    apriltag_ros = Node(
        package='apriltag_ros', executable='apriltag_node',
        parameters=params,
        # El nodo espera image_rect; en Gazebo no hay distorsión, así que image_raw vale
        remappings=[('image_rect', '/camera/image_raw'),
                    ('camera/camera_info', '/camera/camera_info')],
        condition=IfCondition(EqualsSubstitution(detector, 'apriltag_ros')),
        output='screen')

    opencv = Node(
        package='fuga_perception', executable='apriltag_finder',
        parameters=params,
        condition=IfCondition(EqualsSubstitution(detector, 'opencv')),
        output='screen')

    return LaunchDescription([declare_detector, apriltag_ros, opencv])
