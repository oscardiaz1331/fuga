from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription([Node(
        package='apriltag_ros', executable='apriltag_node',
        parameters=[PathJoinSubstitution(
            [FindPackageShare('fuga_perception'), 'config', 'apriltag.yaml']),
            {'use_sim_time': True}],
        # El nodo espera image_rect; en Gazebo no hay distorsión, así que image_raw vale
        remappings=[('image_rect', '/camera/image_raw'),
                    ('camera/camera_info', '/camera/camera_info')],
        output='screen')])
