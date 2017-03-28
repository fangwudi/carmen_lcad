#include "udatmo.h"

#include "detector.h"

#include <stdexcept>

using udatmo::Detector;

static Detector *detector = NULL;

void udatmo_init(const carmen_robot_ackerman_config_t robot_config)
{
	if (detector != NULL)
		throw std::runtime_error("uDATMO module already initialized");

	detector = new Detector(robot_config);
}

bool udatmo_obstacle_detected(double timestamp)
{
	static double last_obstacle_detected_timestamp = 0.0;

	if (detector->detected)
		last_obstacle_detected_timestamp = timestamp;

	if (fabs(timestamp - last_obstacle_detected_timestamp) < 10.0)
		return (true);
	else
		return (false);
}

void udatmo_clear_detected(void)
{
	detector->detected = false;
}

void udatmo_shift_history(void)
{
	detector->shift();
}

int udatmo_detect_obstacle_index(carmen_obstacle_distance_mapper_message *current_map,
							carmen_rddf_road_profile_message *rddf,
							int goal_index,
							int rddf_pose_index,
							carmen_ackerman_traj_point_t robot_pose,
							double timestamp)
{
	int index = detector->detect(current_map, rddf, goal_index, rddf_pose_index, robot_pose, timestamp);

	return (index);
}

double udatmo_speed_front(void)
{
	return detector->speed_front();
}

carmen_ackerman_traj_point_t udatmo_get_moving_obstacle_position(void)
{
	return detector->get_moving_obstacle_position();
}

double udatmo_get_moving_obstacle_distance(carmen_ackerman_traj_point_t robot_pose, carmen_robot_ackerman_config_t *robot_config)
{
	double distance = detector->get_moving_obstacle_distance(robot_pose) - (robot_config->distance_between_front_and_rear_axles + robot_config->distance_between_front_car_and_front_wheels);

	return (distance);
}
