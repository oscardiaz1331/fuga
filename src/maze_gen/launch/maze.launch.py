from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, ExecuteProcess,
                            IncludeLaunchDescription, RegisterEventHandler,
                            Shutdown)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (LaunchConfiguration, PathJoinSubstitution,
                                  PythonExpression)
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
        DeclareLaunchArgument(
            'rtf', default_value='1.0',
            description='Real-time factor maximo: 5 = hasta 5 veces mas rapido '
                        'que el reloj real (si la CPU llega). Todo va con '
                        'use_sim_time, asi que los resultados no cambian'),
        DeclareLaunchArgument(
            'step', default_value='0.001',
            description='Paso de fisica (s). Cada paso tiene un coste fijo: con '
                        '0.001 Gazebo no pasa de ~1.5x aunque rtf sea mayor. '
                        'Con 0.005 llega a ~6x. Mejor un divisor de 0.01 '
                        '(periodo de la IMU) para que los sensores vayan a '
                        'ritmo fijo: 0.001, 0.002 o 0.005'),
        DeclareLaunchArgument(
            'headless', default_value='false',
            description='true para lanzar solo el servidor de Gazebo, sin '
                        'ventana (mas rapido, para baterias de pruebas)'),
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
             '-p', ['start_y:=', start_y],
             # float(): con step:=1 ROS lo leeria como int y el parametro
             # double de maze2world rechazaria el tipo
             '-p', ['step_size:=', PythonExpression(
                 ['float(', LaunchConfiguration('step'), ')'])]],
        output='screen')

    # 2) Cuando termina, lanza Gazebo con el mundo de esa carpeta.
    # Si maze2world falla (p. ej. start fuera del laberinto) se para todo, en
    # vez de abrir Gazebo con el maze.sdf que hubiera de antes
    def on_generated(event, context):
        if event.returncode != 0:
            return Shutdown(reason='maze2world ha fallado (ver el error de arriba)')

        rtf = LaunchConfiguration('rtf').perform(context)
        try:
            rtf_value = float(rtf)
        except ValueError:
            rtf_value = 0.0
        if rtf_value <= 0:
            return Shutdown(reason="rtf tiene que ser un numero > 0, no '%s'" % rtf)
        # -z es cuantos pasos de fisica da Gazebo por segundo de reloj real.
        # Cada paso avanza step segundos (el <physics> que escribe maze2world),
        # asi que rtf / step pasos/s: con step 0.001, 1000 = tiempo real.
        # step ya es valido: si no, maze2world habria fallado
        step = float(LaunchConfiguration('step').perform(context))
        args = '-r -v 3 -z %d' % round(rtf_value / step)
        if LaunchConfiguration('headless').perform(context).lower() == 'true':
            args += ' -s'  # Solo el servidor, sin la GUI
        return IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution([FindPackageShare('ros_gz_sim'),
                                      'launch', 'gz_sim.launch.py'])),
            launch_arguments={
                'gz_args': [args + ' ', PathJoinSubstitution([folder, 'maze.sdf'])]
            }.items())

    start_sim_after_generation = RegisterEventHandler(
        OnProcessExit(target_action=generate_world, on_exit=on_generated))

    return LaunchDescription([
        *declare_args,
        generate_world,
        start_sim_after_generation,
    ])
