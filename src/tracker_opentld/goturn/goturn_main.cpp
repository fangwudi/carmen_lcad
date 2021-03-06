#include <stdio.h>
#include <iostream>
#include <vector>
#include <list>
#include <string>

#include <carmen/carmen.h>
#include <carmen/bumblebee_basic_interface.h>
#include <carmen/bumblebee_basic_messages.h>
#include <carmen/velodyne_interface.h>
#include <carmen/stereo_util.h>
#include <tf.h>

#include <carmen/visual_tracker_interface.h>
#include <carmen/visual_tracker_messages.h>
#include <carmen/velodyne_camera_calibration.h>

#include <carmen/rddf_interface.h>
#include <carmen/rddf_messages.h>

// OpenCV
#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <locale.h>

#include "Smoother/Helpers/wrap2pi.hpp"
#include "Smoother/CGSmoother.hpp"
#include "regressor/regressor_train.h"
#include "g2o/types/slam2d/se2.h"
#include "tracker/tracker.h"
#include "spline.h"
#include "gui.h"

#include "gsl_smooth_points.h"

#include <interpolation.h>
#include <voice.h>

using namespace std;
using namespace cv;
using namespace alglib;

#define BUMBLEBEE_BASIC_VIEW_NUM_COLORS 3
#define MAX_POSES_IN_TRAJECTORY 20
#define DELTA_T (1.0 / 40.0)

struct Image_Box
{
	cv::Mat prev_image;
	BoundingBox prev_box;
	double confidence;
};

rotation_matrix *car_to_global_matrix;
rotation_matrix *sensor_board_to_car_matrix;
rotation_matrix * sensor_to_board_matrix;
carmen_pose_3D_t velodyne_pose;

static int received_image = 0;

std::vector<carmen_velodyne_points_in_cam_t> points_lasers_in_cam;
std::vector<carmen_velodyne_points_in_cam_with_obstacle_t> points_lasers_in_cam_with_obstacle;

carmen_velodyne_partial_scan_message *velodyne_message_arrange;

bool display_all_points_velodyne = true;

static int tld_image_width = 0;
static int tld_image_height = 0;

const float MAX_RANGE = 30.0;
const float MIN_RANGE = 0.5;

carmen_pose_3D_t camera_pose_parameters;
carmen_pose_3D_t board_pose_parameters;
carmen_pose_3D_t velodyne_pose_parameters;

static int camera_side = 0;
int first_matrix = 1;

static int msg_fps = 0, msg_last_fps = 0; //message fps
static int disp_fps = 0, disp_last_fps = 0; //display fps

static carmen_visual_tracker_output_message message_output;

const double fontScale = 2.0;
const int thickness = 3;

int gpu_id = 0;
std::string model_file = "tracker.prototxt";
std::string trained_file = "tracker.caffemodel";
Regressor regressor(model_file, trained_file, gpu_id, false);

int tracker_is_initialized = 0;
int tracker_restart_requested = 0;

const bool show_intermediate_output = false;
Tracker tracker(show_intermediate_output);

static BoundingBox box;
bounding_box box_1;

double average_box_confidence;
static std::string window_name = "GOTURN";
tf::Transformer transformer;
int exit_requested = 0;
int num_prev_frames = 2;

std::vector<Image_Box> last_track;
stereo_util camera_parameters;
carmen_vector_3D_t trackerPoint;
carmen_vector_3D_t target_pose_in_the_world;
std::vector<carmen_localize_ackerman_globalpos_message>  localizeVector;
std::vector<carmen_velodyne_partial_scan_message> velodyne_vector;
smoother::CGSmoother path_smoother;
static unsigned int maxPositions = 100;

static vector<carmen_vector_3D_t> tracker_global_poses;
static vector<carmen_ackerman_traj_point_t> localize_poses_plot;

int show_final_image = 0;
double image_zoom = 1.0 / 2.0;
int show_velodyne_active = 0;
int retain_image_active = 0;
Rect image_temp_bbox(-1, -1, -1, -1);
Rect image_final_bbox(-1, -1, -1, -1);
double avg_range_diff = 0;
double lat_shift = 0;
int quant_points_in_box = 0;
int quant_points_in_box_left = 0;
int quant_points_in_box_right = 0;
int quant_points_in_box_free = 0;
int quant_points_in_box_obstacle = 0;

int marked_width = 1;
int marked_height = 1;

void
plot_state(vector<carmen_vector_3D_t> &points, vector<carmen_ackerman_traj_point_t> &spline,
		vector<carmen_ackerman_traj_point_t> &localize_plot)
{
	static bool first_time = true;
	static FILE *gnuplot_pipeMP;

	if (first_time)
	{
		first_time = false;

		gnuplot_pipeMP = popen("gnuplot", "w");
		fprintf(gnuplot_pipeMP, "set size ratio -1\n");
		fprintf(gnuplot_pipeMP, "set key outside\n");
		fprintf(gnuplot_pipeMP, "set xlabel 'x'\n");
		fprintf(gnuplot_pipeMP, "set ylabel 'y'\n");
	}

	FILE *gnuplot_data_points = fopen("gnuplot_data_points.txt", "w");
	FILE *gnuplot_data_spline = fopen("gnuplot_data_spline.txt", "w");
	FILE *gnuplot_data_localize = fopen("gnuplot_data_localize.txt", "w");

	double x_plot, y_plot;
	double first_x = 7757859.3;
	double first_y = -363559.8;

	for (unsigned int i = 0; i < points.size(); i++)
	{
		x_plot = points[i].x - first_x;
		y_plot = points[i].y - first_y;
		fprintf(gnuplot_data_points, "%lf %lf\n", x_plot, y_plot);
	}
	for (unsigned int i = 0; i < spline.size(); i++)
	{
		x_plot = spline[i].x - first_x;
		y_plot = spline[i].y - first_y;
		fprintf(gnuplot_data_spline, "%lf %lf %lf %lf %lf %lf\n", x_plot, y_plot, 1.0 * cos(spline.at(i).theta),
				1.0 * sin(spline.at(i).theta), spline.at(i).theta, spline.at(i).phi);
	}
	for (unsigned int i = 0; i < localize_plot.size(); i++)
	{
		x_plot = localize_plot[i].x - first_x;
		y_plot = localize_plot[i].y - first_y;
		fprintf(gnuplot_data_localize, "%lf %lf %lf %lf %lf %lf\n", x_plot, y_plot, 1.0 * cos(localize_plot.at(i).theta),
				1.0 * sin(localize_plot.at(i).theta), localize_plot.at(i).theta, localize_plot.at(i).phi);
	}

	fclose(gnuplot_data_points);
	fclose(gnuplot_data_spline);
	fclose(gnuplot_data_localize);

	fprintf(gnuplot_pipeMP, "plot "
			"'./gnuplot_data_points.txt' using 1:2 title 'tracker_points',"
			"'./gnuplot_data_localize.txt' using 1:2 title 'localize',"
			"'./gnuplot_data_spline.txt' using 1:2 title 'spline'\n");

	fflush(gnuplot_pipeMP);
}


//TODO verificar se eh necessario
carmen_ackerman_traj_point_t
move_path_to_current_robot_pose(carmen_vector_3D_t *objectPoint, carmen_ackerman_traj_point_t *localizer_pose)
{
	carmen_ackerman_traj_point_t globalpoint;
	globalpoint.x = localizer_pose->x + objectPoint->x * cos(localizer_pose->theta) - objectPoint->y * sin(localizer_pose->theta);
	globalpoint.y = localizer_pose->y + objectPoint->x * sin(localizer_pose->theta) + objectPoint->y * cos(localizer_pose->theta);
	//	globalpoint.theta = carmen_normalize_theta(objectPoint.theta + localizer_pose->theta);

	return globalpoint;
}



void
plot_to_debug_state(vector<carmen_ackerman_traj_point_t> poses, carmen_vector_3D_t tracker_Point, carmen_ackerman_traj_point_t localize_plot, unsigned int maxPositions)
{
//	if(tracker_global_poses.size() > 1)
//	{
//		tracker_global_poses.back().theta = carmen_normalize_theta(
//				atan2(tracker_global_poses.back().y - tracker_global_poses[tracker_global_poses.size() - 2].y,
//						tracker_global_poses.back().x - tracker_global_poses[tracker_global_poses.size() - 2].x));
//	}

	if (tracker_global_poses.size() > maxPositions)
		tracker_global_poses.erase(tracker_global_poses.begin());

	if (localize_poses_plot.size() > maxPositions)
		localize_poses_plot.erase(localize_poses_plot.begin());

	tracker_global_poses.push_back(tracker_Point); //move_path_to_current_robot_pose(&tracker_Point, &localize_plot));
	localize_poses_plot.push_back(localize_plot);

	if (poses.size() > 0 && tracker_global_poses.size() > 0 && localize_poses_plot.size() > 0)
		plot_state(tracker_global_poses, poses, localize_poses_plot);


}


carmen_vector_3D_t
compute_3d_point(bounding_box box, double range)
{
	double xcenter = (box.x + box.width) / 2;
	double ycenter = (box.y + box.height) / 2;

	double hor_angle, vert_angle;

	assert(camera_parameters.fx > 0);
	assert(camera_parameters.fy > 0);
	assert(range > 0 && range < 1000);

	hor_angle = atan((xcenter - (((double) tld_image_width) / 2.0)) / camera_parameters.fx);
	vert_angle = -atan((ycenter - (((double) tld_image_height) / 2.0)) / camera_parameters.fy);

	double cos_rot_angle = cos(hor_angle);
	double sin_rot_angle = sin(hor_angle);

	double cos_vert_angle = cos(vert_angle);
	double sin_vert_angle = sin(vert_angle);

	double xy_distance = range * cos_vert_angle;

	carmen_vector_3D_t point3d;

	point3d.x = (xy_distance * cos_rot_angle);
	point3d.y = (xy_distance * sin_rot_angle);
	point3d.z = (range * sin_vert_angle);

	return point3d;
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Publishers                                                                                //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////
void
publish_visual_tracker_output_message()
{
	IPC_RETURN_TYPE err;
	err = IPC_publishData(CARMEN_VISUAL_TRACKER_OUTPUT_MESSAGE_NAME, &message_output);
	carmen_test_ipc_exit(err, "Could not publish", CARMEN_VISUAL_TRACKER_OUTPUT_MESSAGE_NAME);
}


void
carmen_visual_tracker_define_messages()
{
	carmen_visual_tracker_define_message_output();
}


void
publish_box_message(char *host, double timestamp)
{
	bounding_box box_detected;

	if (box.x1_ != -1.0)
	{
		box_detected.x = box.x1_;
		box_detected.y = box.y1_;
		box_detected.width = box.get_width();
		box_detected.height = box.get_height();

		message_output.rect = box_detected;
		message_output.confidence = average_box_confidence;
		message_output.host = host;
		message_output.timestamp = timestamp;
	}
	else
	{
		box_detected.x = -1;
		box_detected.y = -1;
		box_detected.width = -1;
		box_detected.height = -1;

		message_output.rect = box_detected;
		message_output.confidence = 0.0;
		message_output.host = host;
		message_output.timestamp = timestamp;
	}

	publish_visual_tracker_output_message();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void
calculate_box_average(BoundingBox &box, std::vector<Image_Box> &last_track, Mat* img, int cont)
{
	BoundingBox average_box;
	average_box.x1_ = 0.0;
	average_box.x2_ = 0.0;
	average_box.y1_ = 0.0;
	average_box.y2_ = 0.0;

	average_box.x1_ += box.x1_;
	average_box.x2_ += box.x2_;
	average_box.y1_ += box.y1_;
	average_box.y2_ += box.y2_;

	//	for(int i = cont%last_track.size(); repeat < limit; i = (i + step)%last_track.size(), repeat++)
	for(unsigned int i = 0; i < last_track.size(); i++)
	{
		tracker.Init(last_track[i].prev_image, last_track[i].prev_box, &regressor);

		tracker.Track(*img, &regressor, &last_track[i].prev_box);

		average_box.x1_ = last_track[i].prev_box.x1_;
		average_box.x2_ = last_track[i].prev_box.x2_;
		average_box.y1_ = last_track[i].prev_box.y1_;
		average_box.y2_ = last_track[i].prev_box.y2_;
		rectangle(*img, cv::Point(average_box.x1_, average_box.y1_),cv::Point(average_box.x2_ , average_box.y2_), Scalar( 0, 255, 0 ), 1, 4 );
		//		average_box.DrawBoundingBox(img);
		last_track[(cont%num_prev_frames)].confidence = 1.0; //
	}

	//	printf("ok\n");
	//	average_box.x1_ /= 3;
	//	average_box.x2_ /= 3;
	//	average_box.y1_ /= 3;
	//	average_box.y2_ /= 3;
	//	box = average_box;
}


cv::Rect
get_mini_box_and_update_carmen_box()
{
	box_1.x = box.x1_;
	box_1.y = box.y1_;
	box_1.width = box.get_width();
	box_1.height = box.get_height();

	cv::Rect mini_box;
	mini_box.x = box_1.x + (box_1.width / 3);
	mini_box.y = box_1.y + (box_1.height / 3);
	mini_box.width = (box_1.width / 3);
	mini_box.height = (box_1.height / 3);

	return (mini_box);
}


std::vector<carmen_vector_3D_t>
select_velodyne_points_inside_box_and_draw_them(const cv::Rect& mini_box, Mat* img, int show_velodyne)
{
	std::vector<carmen_vector_3D_t> points_inside_box;
	int points_in_left = 0;
	int points_in_right = 0;
	int points_in_box_free = 0;
	int points_in_box_obstacle = 0;

//	rectangle(*img,
//			cv::Point(mini_box.x, mini_box.y),
//			cv::Point(mini_box.x + mini_box.width, mini_box.y + mini_box.height),
//			CV_RGB(255, 255, 0), 1, 4);

	//TODO: verificar qual size usar, o total ou so dos hit em obstaculos
	for (unsigned int i = 0; i < points_lasers_in_cam_with_obstacle.size(); i++)
	{

		//verificar se bateu no obstaculo e mostrar na tela
		if(!display_all_points_velodyne && !points_lasers_in_cam_with_obstacle.at(i).hit_in_obstacle)
			continue;

		if ((points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx > box_1.x) &&
				(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx < (box_1.x + box_1.width)) &&
				(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipy > box_1.y)
				&& (points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipy < (box_1.y + box_1.height)))
		{
			if (points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.length <= MIN_RANGE)
				points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.length = MAX_RANGE;

			if (points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.length > MAX_RANGE)
				points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.length = MAX_RANGE;

			//points_lasers_in_cam.at(i).laser_polar.length *= 500;
			//Teste em producao para correcao dos pontos no mundo, nao apagar.
			//			car_to_global_matrix = compute_rotation_matrix(car_to_global_matrix, localizeVector[localizeVector.size() - 1].pose.orientation);
			//			carmen_vector_3D_t point_position_in_the_robot = carmen_get_sensor_sphere_point_in_robot_cartesian_reference(points_lasers_in_cam.at(i).laser_polar,
			//																			velodyne_pose, board_pose_parameters, sensor_to_board_matrix, sensor_board_to_car_matrix);
			//			carmen_vector_3D_t global_point_position_in_the_world = carmen_change_sensor_reference(localizeVector[localizeVector.size() - 1].pose.position,
			//																			point_position_in_the_robot, car_to_global_matrix);
			//
			//			points_inside_box.push_back(global_point_position_in_the_world);

			/*********************************************************************************************************************
			 * IMPORTANTE! (Em duvida, pergunte para o filipe)
			 * EU FACO O ANGULO HORIZONTAL SER NEGATIVO PORQUE OS DADOS DO VELODYNE NAO ESTAO DE ACORDO COM O REFERENCIAL DO
			 * CARMEN, ELES CRESCEM PARA A DIREITA, AO INVES DE CRESCEREM PARA A ESQUERDA. NA HORA DE PROJETAR OS DADOS DO
			 * VELODYNE NA CAMERA, O SINAL FAZ SENTIDO PORQUE AS COLUNAS DA CAMERA TAMBEM CRESCEM PARA A DIREITA.
			 *********************************************************************************************************************/
			points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.horizontal_angle = -points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar.horizontal_angle;
			//printf("hangle: %lf\n", carmen_radians_to_degrees(points_lasers_in_cam.at(i).laser_polar.horizontal_angle));

			points_inside_box.push_back(carmen_covert_sphere_to_cartesian_coord(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.laser_polar));

			if(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx > (box_1.x + (box_1.width/ 2)))
			{
				circle(*img, cv::Point(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx,
						points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipy), 2, Scalar(0, 255, 0), -1);
				points_in_right++;
			}else
			{
				circle(*img, cv::Point(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx,
						points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipy), 2, Scalar(255, 100, 0), -1);
				points_in_left++;
			}

			if(points_lasers_in_cam_with_obstacle.at(i).hit_in_obstacle == true)
				points_in_box_obstacle++;
			else
				points_in_box_free++;

//			circle(*img, cv::Point(points_lasers_in_cam.at(i).ipx, points_lasers_in_cam.at(i).ipy), 2, Scalar(0, 255, 0), -1);

//			if ((points_lasers_in_cam.at(i).ipx > mini_box.x) && (points_lasers_in_cam.at(i).ipx < (mini_box.x + mini_box.width)) &&
//				(points_lasers_in_cam.at(i).ipy > mini_box.y) && (points_lasers_in_cam.at(i).ipy < (mini_box.y + mini_box.height)))
//				circle(*img, cv::Point(points_lasers_in_cam.at(i).ipx, points_lasers_in_cam.at(i).ipy), 2, CV_RGB(255, 255, 255), -1);
		}
		else if (show_velodyne)
			circle(*img, cv::Point(points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipx,
					points_lasers_in_cam_with_obstacle.at(i).velodyne_points_in_cam.ipy), 2, Scalar(0, 255, 255), -1);
	}

	quant_points_in_box_left = points_in_left;
	quant_points_in_box_right = points_in_right;
	quant_points_in_box_free = points_in_box_free;
	quant_points_in_box_obstacle = points_in_box_obstacle;

	return (points_inside_box);
}


//carmen_vector_3D_t
//get_global_point_from_velodyne(carmen_sphere_coord_t sphere_point, velodyne_pose, rotation_matrix *r_matrix_car_to_global, carmen_vector_3D_t robot_position)
//{
//	carmen_vector_3D_t point_position_in_the_robot = carmen_get_sensor_sphere_point_in_robot_cartesian_reference(sphere_point, velodyne_params->pose, sensor_board_1_pose,
//			velodyne_params->sensor_to_board_matrix, sensor_board_1_to_car_matrix);
//
//	carmen_vector_3D_t global_point_position_in_the_world = carmen_change_sensor_reference(robot_position, point_position_in_the_robot, r_matrix_car_to_global);
//
//	return global_point_position_in_the_world;
//}
bool
my_compare_function (carmen_vector_3D_t i, carmen_vector_3D_t j)
{
	return (i.x < j.x);
}


carmen_vector_3D_t
compute_average_point(std::vector<carmen_vector_3D_t> points_inside_box, int begin_best_group, int end_best_group)
{
	carmen_vector_3D_t average_point = {0.0, 0.0, 0.0};

	for (int i = begin_best_group; i < end_best_group; i++)
	{
		average_point.x += points_inside_box[i].x;
		average_point.y += points_inside_box[i].y;
		average_point.z += points_inside_box[i].z;
	}

	double num_points = end_best_group - begin_best_group;

	average_point.x /= (double) (num_points);
	average_point.y /= (double) (num_points);
	average_point.z /= (double) (num_points);

	return (average_point);
}


carmen_vector_3D_t
compute_target_3d_position(std::vector<carmen_vector_3D_t> points_inside_box)
{
	int begin_best_group = 0;
	int end_best_group = 0;
	int group_pivot = 0;
	int num_points_in_the_group = 0;
	int num_points_best_group = 0;
	double dist;

	sort(points_inside_box.begin(), points_inside_box.end(), my_compare_function);

	for (unsigned int i = 0; i < points_inside_box.size(); i++)
	{
		dist = fabs(points_inside_box[i].x - points_inside_box[group_pivot].x);

		if (dist > 0.5)
		{
			if (num_points_in_the_group > num_points_best_group)
			{
				begin_best_group = group_pivot;
				end_best_group = i - 1;
				num_points_best_group = num_points_in_the_group;
			}

			group_pivot = i;
			num_points_in_the_group = 0;
		}
		else
			num_points_in_the_group++;
	}
	//TODO Tratar caso quando segunda parte do grupo nao tem dist > 3
	if (begin_best_group != end_best_group)
		return compute_average_point(points_inside_box, begin_best_group, end_best_group);
	else
		return points_inside_box[begin_best_group];
}


void
bounding_box_interface(char c, Mat* img)
{
	cv::Rect rect;
	switch (c)
	{
		case 'r':

			if (getBBFromUser(img, rect, window_name) == 0)
				return;

			box.x1_ = rect.x;
			box.y1_ = rect.y;
			box.x2_ = rect.x + rect.width;
			box.y2_ = rect.y + rect.height;

			tracker.Init(*img, box, &regressor);
			//carmen_voice_send_alert((char *) "Ah!\n");

			tracker_is_initialized = 1;
			tracker_restart_requested = 1;

			break;

		case 'q':
			exit(0);
	}
}


carmen_velodyne_partial_scan_message
find_velodyne_most_sync_with_cam(double bumblebee_timestamp)
{
	carmen_velodyne_partial_scan_message velodyne;

	double minTimestampDiff = DBL_MAX;
	int minTimestampIndex = -1;

	for (unsigned int i = 0; i < velodyne_vector.size(); i++)
	{
		if(fabs(velodyne_vector[i].timestamp - bumblebee_timestamp) < minTimestampDiff)
		{
			minTimestampIndex = i;
			minTimestampDiff = fabs(velodyne_vector[i].timestamp - bumblebee_timestamp);
		}
	}

	velodyne = velodyne_vector[minTimestampIndex];

	return velodyne;
}




int
call_tracker_and_compute_object_3d_position(Mat *img, int bumblebee_height, int bumblebee_width, double bumblebee_timestamp, carmen_velodyne_partial_scan_message *velodyne_sync_with_cam)
{
	if (box.x1_ != -1.0)
	{
		tracker.Track(*img, &regressor, &box); // calls Thrun's tracker
		cv::Rect mini_box = get_mini_box_and_update_carmen_box();

//		if(display_all_points_velodyne)
//		{
//		points_lasers_in_cam = carmen_velodyne_camera_calibration_lasers_points_in_camera(
//				velodyne_message_arrange, bumblebee_width, bumblebee_height);
//
//		}else
//		{
//			points_lasers_in_cam = carmen_velodyne_camera_calibration_lasers_points_in_camera_with_obstacle(
//							velodyne_message_arrange, bumblebee_width, bumblebee_height);
//		}

		(*velodyne_sync_with_cam) = find_velodyne_most_sync_with_cam(bumblebee_timestamp);
		points_lasers_in_cam_with_obstacle =  carmen_velodyne_camera_calibration_lasers_points_in_camera_with_obstacle_and_display(
				velodyne_sync_with_cam, bumblebee_width, bumblebee_height);

		//TODO: verificar qual size usar, o total ou so dos hit em obstaculos
		if (points_lasers_in_cam_with_obstacle.size() == 0)
		{
			printf("There are not velodyne points in the imagex\n");
			return 0;
		}

		std::vector<carmen_vector_3D_t> points_inside_box =
				select_velodyne_points_inside_box_and_draw_them(mini_box, img, show_velodyne_active);

		quant_points_in_box = points_inside_box.size();
		if (points_inside_box.size() == 0)
		{
			printf("There are not velodyne points inside the box\n");
			return 0;
		}

		trackerPoint = compute_target_3d_position(points_inside_box);
		//box.DrawBoundingBox(img);
	}

	return 1;
}


Mat
image_pre_processing(carmen_bumblebee_basic_stereoimage_message *msg)
{
	static Mat *src_image = NULL;
	static Mat *rgb_image = NULL;

	if (src_image == NULL)
	{
		src_image = new Mat(Size(msg->width, msg->height), CV_8UC3);
		rgb_image = new Mat(Size(msg->width, msg->height), CV_8UC3);
	}

	if (camera_side == 0)
	{
		//src_image->imageData = (char*) msg->raw_left;
		memcpy(src_image->data, msg->raw_left, msg->image_size * sizeof(char));
	}
	else
	{
		//src_image->imageData = (char*) msg->raw_right;
		memcpy(src_image->data, msg->raw_right, msg->image_size * sizeof(char));
	}

	cvtColor(*src_image, *rgb_image, cv::COLOR_RGB2BGR);

	// Mat resized_rgb_image(Size(rgb_image->cols * 0.25, rgb_image->rows * 0.25), CV_8UC3);
	// cv::resize(*rgb_image, resized_rgb_image, resized_rgb_image.size());
	//Mat resized_rgb_image = *rgb_image;
	//return (resized_rgb_image);

	return (*rgb_image);
}


void
initialize_transforms()
{
	tf::Transform velodyne_to_board_pose;
	tf::Transform board_to_car_transform;
	tf::Transform car_to_world_transform;

	tf::Time::init();

	// initial car pose with respect to the world
	//world_to_car_pose.setOrigin(tf::Vector3(car_pose_g.position.x, car_pose_g.position.y, car_pose_g.position.z));
	car_to_world_transform.setOrigin(tf::Vector3(0, 0, 0));
	car_to_world_transform.setRotation(tf::Quaternion(0, 0, 0));
	tf::StampedTransform world_to_car_transform(car_to_world_transform, tf::Time(0), "/world", "/car");
	transformer.setTransform(world_to_car_transform, "world_to_car_transform");

	// board pose with respect to the car
	board_to_car_transform.setOrigin(tf::Vector3(board_pose_parameters.position.x, board_pose_parameters.position.y, board_pose_parameters.position.z));
	board_to_car_transform.setRotation(tf::Quaternion(board_pose_parameters.orientation.yaw, board_pose_parameters.orientation.pitch, board_pose_parameters.orientation.roll)); 				// yaw, pitch, roll
	tf::StampedTransform car_to_board_transform(board_to_car_transform, tf::Time(0), "/car", "/board");
	transformer.setTransform(car_to_board_transform, "car_to_board_transform");

	// velodyne pose with respect to the board
	velodyne_to_board_pose.setOrigin(tf::Vector3(velodyne_pose_parameters.position.x, velodyne_pose_parameters.position.y, velodyne_pose_parameters.position.z));
	velodyne_to_board_pose.setRotation(tf::Quaternion(velodyne_pose_parameters.orientation.yaw, velodyne_pose_parameters.orientation.pitch, velodyne_pose_parameters.orientation.roll)); 				// yaw, pitch, roll
	tf::StampedTransform board_to_velodyne_transform(velodyne_to_board_pose, tf::Time(0), "/board", "/velodyne");
	transformer.setTransform(board_to_velodyne_transform, "board_to_velodyne_transform");
}


tf::StampedTransform
get_transforms_from_velodyne_to_world(double car_x, double car_y, double car_theta)
{
	tf::StampedTransform velodyne_to_world_transform;
	tf::Transform car_to_world_transform;

	car_to_world_transform.setOrigin(tf::Vector3(car_x, car_y, 0.0));
	car_to_world_transform.setRotation(tf::Quaternion(car_theta, 0.0, 0.0));

	tf::StampedTransform car_to_world_stamped_transform(car_to_world_transform, tf::Time(0), "/world", "/car");
	transformer.setTransform(car_to_world_stamped_transform, "car_to_world_stamped_transform");
	transformer.lookupTransform("/world", "/velodyne", tf::Time(0), velodyne_to_world_transform);

	return velodyne_to_world_transform;
}


carmen_vector_3D_t
move_point_from_velodyne_frame_to_world_frame(carmen_vector_3D_t trackerPoint, carmen_ackerman_traj_point_t localize_pose_car)
{
	//posicao da camera em relacao a board
	//posicao da board em relacao ao carro
	//joga o ponto em relacao a essa bagunca
	//carmen_vector_3D_t tracker_car_pose;
	//tracker_car_pose.x = trackerPoint.x + camera_pose_parameters.position.x + board_pose_parameters.position.x;
	//tracker_car_pose.y = trackerPoint.y + camera_pose_parameters.position.y + board_pose_parameters.position.y;
	//tracker_car_pose.z = 0;
	//return tracker_car_pose;

	static int first = 1;

	if (first)
	{
		initialize_transforms();
		first = 0;
	}

	tf::StampedTransform velodyne_to_world = get_transforms_from_velodyne_to_world(
			localize_pose_car.x, localize_pose_car.y, localize_pose_car.theta);

	tf::Vector3 point_velodyne(trackerPoint.x, trackerPoint.y, trackerPoint.z);
	tf::Vector3 point_world = velodyne_to_world * point_velodyne;

	carmen_vector_3D_t tracker_car_pose;
	//
	tracker_car_pose.x = point_world.getX();
	tracker_car_pose.y = point_world.getY();
	tracker_car_pose.z = point_world.getZ();

	//	tracker_car_pose.x = trackerPoint.x;
	//	tracker_car_pose.y = trackerPoint.y;

	//	double tracker_atan = atan2(trackerPoint.y, trackerPoint.x);

	//	tracker_car_pose.x = (trackerPoint.x + board_pose_parameters.position.x + velodyne_pose_parameters.position.x) * cos(localize_pose_car.theta) -
	//						 (trackerPoint.y + board_pose_parameters.position.y + velodyne_pose_parameters.position.y) * sin(localize_pose_car.theta);
	//	tracker_car_pose.y = (trackerPoint.x + board_pose_parameters.position.x + velodyne_pose_parameters.position.x) * sin(localize_pose_car.theta) +
	//						 (trackerPoint.y + board_pose_parameters.position.y + velodyne_pose_parameters.position.y) * cos(localize_pose_car.theta);
	//
	//
	//	tracker_car_pose.x += localize_pose_car.x;
	//	tracker_car_pose.y += localize_pose_car.y;

	return tracker_car_pose;

	//	tf::Point tracker_pose(trackerPoint.x, trackerPoint.y, trackerPoint.z);
	//
	//	tf::Transform pose_camera_in_board(tf::Quaternion(camera_pose_parameters.orientation.yaw,
	//			camera_pose_parameters.orientation.pitch, camera_pose_parameters.orientation.roll),
	//			tf::Vector3(camera_pose_parameters.position.x, camera_pose_parameters.position.y, camera_pose_parameters.position.z));
	//
	//	tf::Transform pose_board(tf::Quaternion(board_pose_parameters.orientation.yaw,
	//			board_pose_parameters.orientation.pitch, board_pose_parameters.orientation.roll),
	//			tf::Vector3(board_pose_parameters.position.x, board_pose_parameters.position.y, board_pose_parameters.position.z));
	//
	//	tf::Transform camera_frame_to_board_frame = pose_camera_in_board;
	//	tf::Transform board_car_frame_ = pose_board.inverse();
	//
	//	tracker_pose = board_car_frame_ * camera_frame_to_board_frame * tracker_pose;
	//carmen_vector_3D_t tacker_car_pose = {tracker_pose.x(), tracker_pose.y(), tracker_pose.z()};

	//	return tracker_car_pose;
}


pair<carmen_ackerman_traj_point_t, double>
find_localize_pose_most_sync_with_velodyne(double timestamp)
{
	pair<carmen_ackerman_traj_point_t, double> stamped_pose;

	double minTimestampDiff = DBL_MAX;
	int minTimestampIndex = -1;

	for (unsigned int i = 0; i < localizeVector.size(); i++)
	{
		if(fabs(localizeVector[i].timestamp - timestamp) < minTimestampDiff)
		{
			minTimestampIndex = i;
			minTimestampDiff = fabs(localizeVector[i].timestamp - timestamp);
		}
	}

	carmen_ackerman_traj_point_t localizePose;

	localizePose.x = localizeVector[minTimestampIndex].globalpos.x;
	localizePose.y = localizeVector[minTimestampIndex].globalpos.y;
	localizePose.theta = localizeVector[minTimestampIndex].globalpos.theta;
	localizePose.v = localizeVector[minTimestampIndex].v;
	localizePose.phi = localizeVector[minTimestampIndex].phi;

	stamped_pose.first = localizePose;
	stamped_pose.second = localizeVector[minTimestampIndex].timestamp;

	return stamped_pose;
}


//TODO Ajustar pesos do otimizador
vector<carmen_ackerman_traj_point_t>
create_smoothed_path_bezier(double timestamp)
{
	(void)timestamp;
	static vector<double> pose_times;
	static vector<double> pose_thetas;
	static unsigned int maxPositions = 20;
	//	int minTimestampIndex = 0;

	static vector<carmen_ackerman_traj_point_t> poses_raw;
	vector<carmen_ackerman_traj_point_t> poses_filtered;
	vector<carmen_ackerman_traj_point_t> poses_smooth;

	carmen_ackerman_traj_point_t localizePose;
	carmen_vector_3D_t target_pose_in_the_world;

	if(localizeVector.size() < 1)
		return poses_smooth;

	//	minTimestampIndex = sincronized_localize_pose_with_velodyne();

	double minTimestampDiff = DBL_MAX;
	int minTimestampIndex = -1;

	for (unsigned int i = 0; i < localizeVector.size(); i++)
	{
		if(fabs(localizeVector[i].timestamp - velodyne_message_arrange->timestamp) < minTimestampDiff)
		{
			minTimestampIndex = i;
			minTimestampDiff = fabs(localizeVector[i].timestamp - velodyne_message_arrange->timestamp);
		}
	}

	localizePose.x = localizeVector[minTimestampIndex].globalpos.x;
	localizePose.y = localizeVector[minTimestampIndex].globalpos.y;
	localizePose.theta = localizeVector[minTimestampIndex].globalpos.theta;
	localizePose.v = localizeVector[minTimestampIndex].v;
	localizePose.phi = localizeVector[minTimestampIndex].phi;

	//    printf("LOCALIZE FOUND: %d DIFF: %lf POSE %lf %lf TIME: %lf\n", minTimestampIndex, minTimestampDiff, localizeVector[minTimestampIndex].globalpos.x,
	//            localizeVector[minTimestampIndex].globalpos.y, localizeVector[minTimestampIndex].timestamp);

	target_pose_in_the_world = move_point_from_velodyne_frame_to_world_frame(trackerPoint, localizePose);


	if ((poses_raw.size() == 0) || (poses_raw[poses_raw.size() - 1].x != target_pose_in_the_world.x))
	{
		carmen_ackerman_traj_point_t pose_temp;

		pose_times.push_back(localizeVector[minTimestampIndex].timestamp);
		pose_thetas.push_back(localizePose.theta);


		pose_temp.x = target_pose_in_the_world.x;
		pose_temp.y = target_pose_in_the_world.y;

		poses_raw.push_back(pose_temp);
	}

	if (poses_raw.size() > maxPositions)
	{
		poses_raw.erase(poses_raw.begin());
		pose_times.erase(pose_times.begin());
		pose_thetas.erase(pose_thetas.begin());
	}

	// FIXME Incluir as modificacoes de correcao de pontos

	//	printf("\n\n-----------------------\n");

	//vai ser armazenado em poses_filtered
	std::vector<double> Xteste;
	std::vector<double> Yteste;
	std::vector<double> Tteste;

	double first_pose_x = localizeVector[localizeVector.size() - 1].globalpos.x;
	double first_pose_y = localizeVector[localizeVector.size() - 1].globalpos.y;
	double first_pose_theta = localizeVector[localizeVector.size() - 1].globalpos.theta;

	Xteste.push_back(0.0);
	Yteste.push_back(0.0);
	Tteste.push_back(0.0);

	//	double target_x = 0.0;
	//	double target_y = 0.0;

	//double angle = 0.0;
	//double angle_diff = 0.0;

	g2o::SE2 robot_pose(first_pose_x, first_pose_y, first_pose_theta);
	g2o::SE2 last_pose(first_pose_x, first_pose_y, first_pose_theta);

	for (unsigned int i = 0; i < poses_raw.size(); i++)
	{
		//		double pix = X[i] - first_pose_x;
		//		double piy = Y[i] - first_pose_y;

		g2o::SE2 target_in_world_reference(poses_raw[i].x, poses_raw[i].y, 0);
		g2o::SE2 target_in_last_pose_reference = last_pose.inverse() * target_in_world_reference;
		g2o::SE2 target_in_car_reference = robot_pose.inverse() * target_in_world_reference;

		//		target_x = target_in_car_reference[0];
		//		target_y = target_in_car_reference[1];
		//		angle = fabs(carmen_radians_to_degrees(atan2(piy - Yspline.back(), pix - Xspline.back())));

		double angle_in_the_world = atan2(poses_raw[i].y - last_pose[1], poses_raw[i].x - last_pose[0]);
		double angle_in_car_reference = atan2(target_in_car_reference[1], target_in_car_reference[0]);
		double angle_from_last_pose = atan2(target_in_last_pose_reference[1], target_in_last_pose_reference[0]);

		//if(Tteste.size() > 2)
		//angle_diff = fabs(carmen_radians_to_degrees(carmen_normalize_theta(mrpt::math::angDistance(angle, Tteste.back()))));

		//printf("X: %lf  Y: %lf angle: %lf\n",target_x, target_y, angle);
		//printf("Xback: %lf  Yback: %lf angleDiff: %lf size: %d\n\n", Xteste.back(), Yteste.back(), angle_diff, Tteste.size());

		//		double dt = (pose_times[i] - pose_times[i - 1]);
		//		double dist = sqrt(pow(piy - pi_1y, 2) + pow(pix - pi_1x, 2));

		if ((fabs(carmen_radians_to_degrees(angle_from_last_pose)) > 20.0) /*|| (angle_diff > 20.0)*/
				|| (target_in_last_pose_reference[0] < 0)
				|| target_in_car_reference[0] < 4.5)
		{
			poses_raw.erase(poses_raw.begin() + i);
			continue;
		}

		Xteste.push_back(target_in_car_reference[0]);
		Yteste.push_back(target_in_car_reference[1]);
		Tteste.push_back(angle_in_car_reference); // TODO: CORRIGIR ESSE ANGULO. ELE ESTA NA REF. DO PONTO ANTERIOR, MAS ELE DEVERIA ESTAR NA REF. DO CARRO.

		last_pose = g2o::SE2(poses_raw[i].x, poses_raw[i].y, angle_in_the_world);
		printf("\tX: %lf Y: %lf T: %lf\n", target_in_car_reference[0], target_in_car_reference[1], angle_in_car_reference);
	}

	printf("Xteste size: %ld\n", Xteste.size());

	//getchar();
	carmen_ackerman_traj_point_t pose_temp;
	for(unsigned int i = 0; i < Xteste.size(); i++)
	{

		// TODO: MOVER OS PONTOS FILTRADOS PARA A REFERENCIA DO MUNDO.
		g2o::SE2 point_in_car_reference(Xteste[i], Yteste[i], 0.0);
		g2o::SE2 point_in_the_world = robot_pose * point_in_car_reference;

		pose_temp.x = point_in_the_world[0];
		pose_temp.y = point_in_the_world[1];
		pose_temp.theta = point_in_the_world[2];
		pose_temp.phi = 0.0;
		pose_temp.v = 0.0;
		poses_filtered.push_back(pose_temp);
	}
	//	printf("\n\n----------poses_filtered: %ld-------------\n", poses_filtered.size());
	if (poses_filtered.size() > 5)
	{
		poses_smooth = path_smoother.Smooth(poses_filtered);
	}

	// CHECAR POR QUE CHEGOU COM 0 AQUI
	if (poses_smooth.size() > 0)
		poses_smooth.pop_back();

	//for (unsigned int i = 0; i < poses.size(); i++)
	//printf("X: %lf Y: %lf TH: %lf\n", poses[i].x, poses[i].y, poses[i].theta);
	//printf("\n\n-----------------------\n");

	//	for (unsigned int i = 0; i < poses.size(); i++)
	//		printf("X: %lf Y: %lf TH: %lf\n", poses[i].x, poses[i].y, poses[i].theta);
	//	printf("\n\n-----------------------\n");

	plot_to_debug_state(poses_smooth, target_pose_in_the_world, localizePose, 100);

	return poses_smooth;
}


int
target_point_in_relation_to_last_target_detection_is_valid(
		carmen_vector_3D_t target_pose,
		carmen_vector_3D_t last_pose,
		double last_pose_angle)
{
	//g2o::SE2 target_transform(target_pose.x, target_pose.y, 0);
	//g2o::SE2 last_pose_transform(last_pose.x, last_pose.y, last_pose_angle);
	//g2o::SE2 target_in_last_pose_reference = last_pose_transform.inverse() * target_transform;
	//double angle_from_last_pose = atan2(target_in_last_pose_reference[1], target_in_last_pose_reference[0]);

	double angle_from_last_pose = atan2(target_pose.y - last_pose.y, target_pose.x - last_pose.x);
	double diff = carmen_normalize_theta(angle_from_last_pose - last_pose_angle);

	//diff = mrpt::math::angDistance(angle_from_last_pose, last_pose_angle);

	//if (fabs(carmen_radians_to_degrees(angle_from_last_pose)) > 30.0)
	if (carmen_radians_to_degrees(fabs(diff)) > 30.0)
	{
		//printf("REJECTING POINT DUE TO INVALID ANGLE VARIATION: %lf\n", carmen_radians_to_degrees(fabs(diff)));
		return 0;
	}

	double dist = sqrt(pow(target_pose.x - last_pose.x, 2) + pow(target_pose.y - last_pose.y, 2));

	if (dist < 1.0)
		return 0;

	//	printf("ACCEPTING POINT DUE TO INVALID ANGLE VARIATION: TP Y: %lf LP Y: %lf TP X: %lf LP X: %lf ANGLE: %lf\n",
	//			target_pose.y, last_pose.y, target_pose.x, last_pose.x, carmen_radians_to_degrees(fabs(diff)));

	//	if (target_in_last_pose_reference[0] < 0)
	//	{
	//		printf("REJECTING POINT DUE TO X VARIATION: %lf\n", target_in_last_pose_reference[0]);
	//		return 0;
	//	}

	return 1;
}


int
point_is_valid(carmen_vector_3D_t target_pose_in_the_world, carmen_ackerman_traj_point_t localize_pose)
{
	static int first = 1;
	static carmen_vector_3D_t last_target_pose;
	static double last_pose_angle = 0;

	if (first || tracker_restart_requested)
	{
		last_target_pose.x = localize_pose.x;
		last_target_pose.y = localize_pose.y;
		last_target_pose.z = 0;
		last_pose_angle = localize_pose.theta;
		first = 0;
	}

	// @Filipe: se o codigo nao entrar no if abaixo por muito tempo, vai dar problema...
	if (target_point_in_relation_to_last_target_detection_is_valid(
			target_pose_in_the_world,
			last_target_pose,
			last_pose_angle))
	{
		last_pose_angle = atan2(
				target_pose_in_the_world.y - last_target_pose.y,
				target_pose_in_the_world.x - last_target_pose.x);

		last_target_pose = target_pose_in_the_world;

		return 1;
	}

	return 0;
}


pair<barycentricinterpolant, barycentricinterpolant>
build_interpolation(vector<carmen_vector_3D_t> target_points, carmen_ackerman_traj_point_t localize_pose, vector<double> target_point_times, double localize_timestamp)
{
	ae_int_t info;
	ae_int_t polynomial_order;
	barycentricinterpolant px, py;
	polynomialfitreport rep;

	std::vector<double> vt;
	std::vector<double> vx;
	std::vector<double> vy;
	std::vector<double> vw;

	//	if (localize_pose.v < 1) polynomial_order = 2;
	//	else if (localize_pose.v < 3.0) polynomial_order = 3;
	//	else polynomial_order = 5;
	polynomial_order = 5;

	g2o::SE2 pose_t (0,0,localize_pose.theta);

	if (localize_pose.v < 1)
	{
		vt.push_back(target_point_times[0] - localize_timestamp - 1);
		vx.push_back(0);
		vy.push_back(0);
		vw.push_back(10);

		//polynomial_order = 2;
	}

	for (uint i = 0; i < target_points.size(); i++)
	{
		g2o::SE2 point_t(target_points[i].x - localize_pose.x, target_points[i].y - localize_pose.y, 0);
		g2o::SE2 point_in_car_t = pose_t.inverse() * point_t;

		vt.push_back(target_point_times[i] - localize_timestamp);
		//vx.push_back(target_points[i].x - localize_pose.x);
		//vy.push_back(target_points[i].y - localize_pose.y);
		vx.push_back(point_in_car_t[0]);
		vy.push_back(point_in_car_t[1]);
		vw.push_back(1);
	}

	for (uint i = 0; i < vt.size(); i++)
	{
		printf("%lf\t", vt[i]);
		printf("%lf\t", vx[i]);
		printf("%lf\n", vy[i]);
	}

	printf("--------------------------\n");

	real_1d_array x;
	real_1d_array y;
	real_1d_array t;
	real_1d_array w;

	vw[vw.size() - 1] = 10;

	x.setcontent(vx.size(), &(vx[0]));
	y.setcontent(vy.size(), &(vy[0]));
	t.setcontent(vt.size(), &(vt[0]));
	w.setcontent(vw.size(), &(vw[0]));

	// polinomial fit without weights
	// polynomialfit(t, x, polynomial_order, info, px, rep);
	// polynomialfit(t, y, polynomial_order, info, py, rep);

	// polinomial fit with weights
	polynomialfitwc(t, x, w, "[]", "[]", "[]", polynomial_order, info, px, rep);
	printf("FIT ERROR X: %ld\n", (long) info);
	polynomialfitwc(t, y, w, "[]", "[]", "[]", polynomial_order, info, py, rep);
	printf("FIT ERROR Y: %ld\n", (long) info);

	return pair<barycentricinterpolant, barycentricinterpolant>(px, py);
}


barycentricinterpolant
build_interpolation2(vector<carmen_vector_3D_t> target_points, carmen_ackerman_traj_point_t localize_pose)
{
	ae_int_t info;
	ae_int_t polynomial_order;
	barycentricinterpolant px, py;
	polynomialfitreport rep;

	std::vector<double> vx;
	std::vector<double> vy;
	std::vector<double> vw;

	if (localize_pose.v < 1) polynomial_order = 2;
	else if (localize_pose.v < 3.0) polynomial_order = 3;
	else polynomial_order = 5;

	g2o::SE2 pose_t (0,0,localize_pose.theta);

	vx.push_back(0);
	vy.push_back(0);
	vw.push_back(10);

	for (uint i = 0; i < target_points.size(); i++)
	{
		g2o::SE2 point_t (target_points[i].x - localize_pose.x, target_points[i].y - localize_pose.y, 0);
		g2o::SE2 point_in_car_t = pose_t.inverse() * point_t;

		//vx.push_back(target_points[i].x - localize_pose.x);
		//vy.push_back(target_points[i].y - localize_pose.y);
		vx.push_back(point_in_car_t[0]);
		vy.push_back(point_in_car_t[1]);
		vw.push_back(1);
	}

	for (uint i = 0; i < vx.size(); i++)
	{
		printf("%lf\t", vx[i]);
		printf("%lf\n", vy[i]);
	}

	printf("--------------------------\n");

	real_1d_array x;
	real_1d_array y;
	real_1d_array t;
	real_1d_array w;

	x.setcontent(vx.size(), &(vx[0]));
	y.setcontent(vy.size(), &(vy[0]));
	w.setcontent(vw.size(), &(vw[0]));

	// polinomial fit without weights
	// polynomialfit(t, x, polynomial_order, info, px, rep);
	// polynomialfit(t, y, polynomial_order, info, py, rep);

	// polinomial fit with weights
	polynomialfitwc(x, y, w, "[]", "[]", "[]", polynomial_order, info, px, rep);
	printf("FIT ERROR: %ld\n", (long) info);

	return px;
}


vector<carmen_ackerman_traj_point_t>
create_smoothed_path3(double timestamp_image)
{
	static vector<double> pose_times;
	static vector<double> pose_thetas;
	static unsigned int maxPositions = 20;
	//	int minTimestampIndex = 0;

	static vector<carmen_vector_3D_t> target_points;
	static vector<double> target_point_times;
	vector<carmen_ackerman_traj_point_t> poses;

	carmen_vector_3D_t target_pose_in_the_world;

	if(localizeVector.size() < 1)
		return vector<carmen_ackerman_traj_point_t>();

	pair<carmen_ackerman_traj_point_t, double> sync_pose_and_time;

	sync_pose_and_time = find_localize_pose_most_sync_with_velodyne(timestamp_image);
	target_pose_in_the_world = move_point_from_velodyne_frame_to_world_frame(trackerPoint, sync_pose_and_time.first);

	if (point_is_valid(target_pose_in_the_world, sync_pose_and_time.first))
	{
		target_points.push_back(target_pose_in_the_world);
		target_point_times.push_back(timestamp_image);
	}

	//	if ((poses_raw.size() == 0) || (poses_raw[poses_raw.size() - 1].x != target_pose_in_the_world.x))
	//	{
	//		carmen_ackerman_traj_point_t pose_temp;
	//
	//		pose_times.push_back(sync_pose_and_time.second);
	//		pose_thetas.push_back(localizePose.theta);
	//
	//		pose_temp.x = target_pose_in_the_world.x;
	//		pose_temp.y = target_pose_in_the_world.y;
	//
	//		poses_raw.push_back(pose_temp);
	//	}

	if (target_points.size() < 2)
		return vector<carmen_ackerman_traj_point_t>();

	if (target_points.size() > maxPositions)
	{
		target_points.erase(target_points.begin());
		target_point_times.erase(target_point_times.begin());

		//		poses_raw.erase(poses_raw.begin());
		//		pose_times.erase(pose_times.begin());
		//		pose_thetas.erase(pose_thetas.begin());
	}

	pair<barycentricinterpolant, barycentricinterpolant> poly_x_and_poly_y = build_interpolation(
			target_points, sync_pose_and_time.first, target_point_times, sync_pose_and_time.second);

	barycentricinterpolant px = poly_x_and_poly_y.first;
	barycentricinterpolant py = poly_x_and_poly_y.second;

	unsigned int i;
	carmen_ackerman_traj_point_t pose;

	// 0 because the localize time is the reference for subtraction
	pose.x = sync_pose_and_time.first.x;
	pose.y = sync_pose_and_time.first.y;
	pose.theta = 0.0;
	pose.phi = 0.0;
	pose.v = 1.0;

	poses.push_back(pose);

	g2o::SE2 pose_t(0,0,sync_pose_and_time.first.theta);

	for(i = 0; i < target_point_times.size(); i++)
	{
		//pose.x = barycentriccalc(px, target_point_times[i] - sync_pose_and_time.second) + sync_pose_and_time.first.x;
		//pose.y = barycentriccalc(py, target_point_times[i] - sync_pose_and_time.second) + sync_pose_and_time.first.y;

		g2o::SE2 point_t_car (
				barycentriccalc(px, target_point_times[i] - sync_pose_and_time.second),
				barycentriccalc(py, target_point_times[i] - sync_pose_and_time.second),
				0);

		g2o::SE2 point_t = pose_t * point_t_car;

		pose.x = point_t[0] + sync_pose_and_time.first.x;
		pose.y = point_t[1] + sync_pose_and_time.first.y;

		pose.theta = 0.0;
		pose.phi = 0.0;
		pose.v = 1.0;

		poses.push_back(pose);
	}

	for(i = 0; i < poses.size(); i++)
	{
		if (i == (poses.size() - 1))
		{
			poses[i].theta = poses[i - 1].theta;
			printf("i: %d theta: %lf\n", i, poses[i].theta);
		}
		else
		{
			poses[i].theta = atan2(poses[i + 1].y - poses[i].y, poses[i + 1].x - poses[i].x);

			printf("i: %d i+1: %lf %lf i: %lf %lf theta: %lf dx: %lf dy: %lf\n", i, poses[i + 1].x, poses[i + 1].y, poses[i].x, poses[i].y, poses[i].theta,
					poses[i + 1].x - poses[i].x, poses[i + 1].y - poses[i].y);
		}
	}

	plot_to_debug_state(poses, target_pose_in_the_world, sync_pose_and_time.first, 100);
	return poses;
}


//void
//move_target_pose_backwards(carmen_ackerman_traj_point_t target_pose, carmen_ackerman_traj_point_t &target_pose_moved, carmen_ackerman_traj_point_t localize)
//{
//	g2o::SE2 tp_transf(target_pose.x, target_pose.y, 0);
//	g2o::SE2 local_transf(localize.x, localize.y, localize.theta);
//	g2o::SE2 tm_transf = local_transf.inverse() * tp_transf;
//	g2o::SE2 moved_tm = g2o::SE2(tm_transf[0] - 7, tm_transf[1], tm_transf[2]);
//	tm_transf = local_transf * moved_tm;
//
//	target_pose_moved.x = tm_transf[0];
//	target_pose_moved.y = tm_transf[1];
//	target_pose_moved.theta = target_pose.theta;
//	target_pose_moved.v = target_pose.v;
//	target_pose_moved.phi = target_pose.phi;
//}

//void
//get_motion_control(carmen_ackerman_traj_point_t prev, carmen_ackerman_traj_point_t next, double speed, double elapsed_time)
//{
//	double w_v_error = 0.0;
//	double w_v_past_error = 0.0;
//	double how_far;
//	double desired_speed;
//	double v_error, v_past_error, v_total_error;
//	double brake, gas;
//
//	how_far = 0.0;
//	desired_speed = (prev.v * how_far) + (prev.v * (1 - how_far));
//	v_error = speed - desired_speed;
//	v_past_error = v_error * elapsed_time;
//	v_total_error = (w_v_error * v_error) + (w_v_past_error * v_past_error);
//
//	if (v_total_error >0){
//
//	}
//}

// get the desired speed
double
get_curvature_constraint(carmen_ackerman_traj_point_t prev, carmen_ackerman_traj_point_t current, carmen_ackerman_traj_point_t next)
{

	// get the appropriated displacement vectors
	carmen_ackerman_traj_point_t dxi;
	carmen_ackerman_traj_point_t dxip1;

	dxi.x = current.x - prev.x;
	dxi.y = current.y - prev.y;

	dxip1.x = next.x - current.x;
	dxip1.y = next.y - current.y;

	// get the angle between the two vectors
	double angle = std::fabs(mrpt::math::angDistance<double>(atan2(dxip1.y, dxip1.x), atan2(dxi.y, dxi.x)));

	// get the turn radius
	double radius = std::sqrt(dxi.x*dxi.x + dxi.y*dxi.y) / angle;

	// get the curvature constraint
	return sqrt(radius * 0.4);

}

void
correct_thetas(vector<carmen_ackerman_traj_point_t> &target_poses)
{
	for(unsigned int i = 1; i < target_poses.size() - 1; i++ )
	{
		target_poses[i].theta = atan2(
				target_poses[i + 1].y - target_poses[i - 1].y,
				target_poses[i + 1].x - target_poses[i - 1].x);
	}

	target_poses.at(target_poses.size()-1).theta = target_poses.at(target_poses.size()-2).theta;
}


vector<carmen_ackerman_traj_point_t>
filter_trajectory(vector<carmen_ackerman_traj_point_t> &target_poses, carmen_ackerman_traj_point_t localize_sync)
{
	vector<carmen_ackerman_traj_point_t> target_poses_new;
	carmen_ackerman_traj_point_t target = target_poses[target_poses.size() - 1];

	target_poses_new.push_back(localize_sync);

	for (std::vector<carmen_ackerman_traj_point_t>::iterator it = target_poses.begin(); it != target_poses.end(); ++it)
	{
		g2o::SE2 robot_pose(localize_sync.x, localize_sync.y ,localize_sync.theta);
		g2o::SE2 target_in_world_reference(it->x, it->y ,it->theta);
		g2o::SE2 target_in_robot_reference = robot_pose.inverse() * target_in_world_reference;

		double dist_to_localize = sqrt(pow(target_in_robot_reference[0], 2) + pow(target_in_robot_reference[1], 2));
		double dist_to_target = sqrt(pow(it->x - target.x, 2) + pow(it->y - target.y, 2));

		if (target_in_robot_reference[0] >= 0.5 && dist_to_target >= 7.0 && dist_to_localize > 0.01)
			target_poses_new.push_back(*it);
	}

	if (target_poses_new.size() <= 2) // caso inicial: adiciona poses entre o localizer e a posicao do target
	{
		double n_slices;
		carmen_ackerman_traj_point_t target_pose;

		n_slices = sqrt(pow(localize_sync.x - target.x, 2) + pow(localize_sync.y - target.y, 2)) / 0.5;
		target_pose = target;

		vector<carmen_ackerman_traj_point_t> target_poses_new_with_additional_points;
		carmen_ackerman_traj_point_t goal_pose = localize_sync;

		double dx = target_pose.x - localize_sync.x;
		double dy = target_pose.y - localize_sync.y;
		double delta_x = dx / n_slices;
		double delta_y = dy / n_slices;

		double dist_to_target = sqrt(pow(target_pose.x - goal_pose.x, 2) + pow(target_pose.y - goal_pose.y, 2));

		while (dist_to_target > 7.0)
		{
			target_poses_new_with_additional_points.push_back(goal_pose);

			goal_pose.x += delta_x;
			goal_pose.y += delta_y;

			dist_to_target = sqrt(pow(target_pose.x - goal_pose.x, 2) + pow(target_pose.y - goal_pose.y, 2));
		}

		return target_poses_new_with_additional_points;
	}


	//	if (target_poses_new.size() > 2) // caso em que a distancia para o 1o target eh problematico
	//	{
	//		double dist_to_first_target = sqrt(pow(target_poses_new[1].x - localize_sync.x, 2) + pow(target_poses_new[1].y - localize_sync.y, 2));
	//
	//		if (dist_to_first_target < 1.1)
	//			return target_poses_new;
	//
	//		double n_slices;
	//		carmen_ackerman_traj_point_t target_pose;
	//
	//		n_slices = dist_to_first_target / 0.5;
	//		target_pose = target_poses_new[1];
	//
	//		vector<carmen_ackerman_traj_point_t> target_poses_new_with_additional_points;
	//		carmen_ackerman_traj_point_t goal_pose = localize_sync;
	//
	//		double dx = target_pose.x - localize_sync.x;
	//		double dy = target_pose.y - localize_sync.y;
	//		double delta_x = dx / n_slices;
	//		double delta_y = dy / n_slices;
	//
	//		int i;
	//
	//		for (i = 0; i < (n_slices + 1); i++)
	//		{
	//			target_poses_new_with_additional_points.push_back(goal_pose);
	//
	//			goal_pose.x += delta_x;
	//			goal_pose.y += delta_y;
	//		}
	//
	//		for (i = 1; i < target_poses_new.size(); i++)
	//			target_poses_new_with_additional_points.push_back(target_poses_new[i]);
	//
	//		return target_poses_new_with_additional_points;
	//	}


	//	if (target_poses.size() == 2 && robot_in_start_position)
	//	{
	//		carmen_ackerman_traj_point_t target_pose_first = sync_pose_and_time.first;
	//
	//		double dx = target_pose_in_the_world.x - sync_pose_and_time.first.x;
	//		double dy = target_pose_in_the_world.y - sync_pose_and_time.first.y;
	//		double delta_x = dx / 10.0;
	//		double delta_y = dy / 10.0;
	//
	//		while (dist_robot_to_target > 3.0)
	//		{
	//			target_pose_first.x += delta_x;
	//			target_pose_first.y += delta_y;
	//			dist_robot_to_target = sqrt(pow((target_pose_in_the_world.x - target_pose_first.x),2) + pow((target_pose_in_the_world.y - target_pose_first.y),2));
	//			target_poses_new.push_back(target_pose_first);
	//		}
	//		robot_in_start_position = 0;
	//	}
	//
	//	double distance_last_goal_to_target = sqrt(pow((target_poses_new.back().x - target_poses.back().x),2) + pow((target_poses_new.back().y - target_poses.back().y),2));
	//
	////	printf("dist: %lf size: %ld \n", distance_last_goal_to_target, target_poses_new.size());
	//	if (distance_last_goal_to_target > 2.0)
	//	{
	//		target_poses_new.push_back(target_poses.back());
	////		printf("add point in new x: %lf y: %lf \n", target_poses.back().x, target_poses.back().y);
	//
	//	}

	return target_poses_new;
}


void
compute_goal_velocity(vector<carmen_ackerman_traj_point_t> &target_poses, vector<double> times, double &sum,
		carmen_ackerman_traj_point_t localize)
{
	(void) sum;

	if(target_poses.size() > 6)
	{
		double x = target_poses[target_poses.size() - 1].x;
		double y = target_poses[target_poses.size() - 1].y;

		double x_1 = target_poses[target_poses.size() - 6].x;
		double y_1 = target_poses[target_poses.size() - 6].y;

		double dist = sqrt(pow(x - x_1,2) + pow(y - y_1,2));
		double dt = fabs(times[times.size() - 1] - times[times.size() - 6]);

		double dist_to_localize = sqrt(pow(x - localize.x, 2) + pow(y - localize.y, 2));
		double multiplier;

		// caso tenha duvida sobre os valores abaixo, converse com luan
		if (dist_to_localize < 8.0) multiplier = 0.0;
		else if (dist_to_localize < 13) multiplier = 0.2 * (dist_to_localize - 8);
		else if (dist_to_localize < 21) multiplier = 1.0 + (0.03125 * (dist_to_localize - 13));
		else multiplier = 1.25;

		if (dt == 0 /*|| dist < 7.5*/)
			target_poses[target_poses.size() - 1].v = 0.0;
		else
			target_poses[target_poses.size() - 1].v = (dist / dt) * multiplier;


		if (target_poses[target_poses.size() - 1].v > 30.0)
			target_poses[target_poses.size() - 1].v = 30.0;

		printf("Dist to localize: %lf multiplier: %lf velocity: %lf velocity multiplied: %lf car velocity: %lf\n",
				dist_to_localize, multiplier, 3.6 * (dist / dt), target_poses[target_poses.size() - 1].v * 3.6,
				localize.v * 3.6);
	}
	else
	{
		target_poses[target_poses.size() - 1].v = 0.0; //4.0;
	}

//	if(target_poses[target_poses.size() - 1].v != 0)
//	{
//	sum += target_poses[target_poses.size() - 1].v;
//	target_poses[target_poses.size() - 1].v = sum / target_poses.size();
//	}
}


void
add_point_to_trajectory_and_compute_smooth_trajectory(carmen_vector_3D_t target_pose, double image_timestamp,
		vector<carmen_ackerman_traj_point_t> *trajectory, vector<double> *timestamps,
		vector<carmen_ackerman_traj_point_t> *smooth_trajectory, carmen_ackerman_traj_point_t localize_pose,
		double sum)
{
	carmen_ackerman_traj_point_t trajectory_pose;

	trajectory_pose.x = target_pose.x;
	trajectory_pose.y = target_pose.y;

	trajectory_pose.theta = atan2(
			target_pose.y - trajectory->at(trajectory->size() - 1).y,
			target_pose.x - trajectory->at(trajectory->size() - 1).x);

	trajectory_pose.phi = 0;

	trajectory->push_back(trajectory_pose);
	timestamps->push_back(image_timestamp);

	//smooth_points
	//(*smooth_trajectory) = smooth_points(*trajectory);
	//compute_goal_velocity(*smooth_trajectory, *timestamps, sum, localize_pose);
	compute_goal_velocity(*trajectory, *timestamps, sum, localize_pose);

	if (trajectory->size() > MAX_POSES_IN_TRAJECTORY)
	{
		trajectory->erase(trajectory->begin());
		timestamps->erase(timestamps->begin());
	}

	if (smooth_trajectory->size() > MAX_POSES_IN_TRAJECTORY)
		smooth_trajectory->erase(smooth_trajectory->begin());
}


vector<carmen_ackerman_traj_point_t>
build_trajectory(double image_timestamp, carmen_velodyne_partial_scan_message *velodyne_sync_with_cam)
{
	static vector<double> pose_times;
	static vector<double> pose_thetas;
	static vector<carmen_ackerman_traj_point_t> trajectory;
	static vector<double> times;
	static double sum = 0.0;
	static double time_last_pose_was_added = 0;
	pair<carmen_ackerman_traj_point_t, double> pose_and_time;
	vector<carmen_ackerman_traj_point_t> smooth_trajectory;
	int point_added = 0;

	if (tracker_restart_requested)
	{
		pose_times.clear();
		pose_thetas.clear();
		trajectory.clear();
		times.clear();
		sum = 0.0;
		time_last_pose_was_added = 0;
	}

	if(localizeVector.size() < 1)
		return vector<carmen_ackerman_traj_point_t>();

	// busca a mensagem do localizer mais proxima
	pose_and_time = find_localize_pose_most_sync_with_velodyne(velodyne_sync_with_cam->timestamp);
	target_pose_in_the_world = move_point_from_velodyne_frame_to_world_frame(trackerPoint, pose_and_time.first);

	// se for a primeira pose, adiciona a pose do localize no inicio.
	if (trajectory.size() == 0)
	{
		trajectory.push_back(pose_and_time.first);
		times.push_back(image_timestamp);
	}

// Print to experiment data	
/*	printf("TARGET_POSE_RAW x: %lf y: %lf timestamp: %lf\n",
			target_pose_in_the_world.x,
			target_pose_in_the_world.y,
			image_timestamp);
*/

	// se (o ponto no mundo for valido) ou (nenhum ponto foi adicionado nos ultimos 0.5
	// segundos e o carro esta se movendo)
	if (point_is_valid(target_pose_in_the_world, pose_and_time.first) ||
		((fabs(pose_and_time.second - time_last_pose_was_added) > 0.5) &&
			(pose_and_time.first.v > 0.5)))
	{
		add_point_to_trajectory_and_compute_smooth_trajectory(target_pose_in_the_world, image_timestamp,
				&trajectory, &times, &smooth_trajectory, pose_and_time.first, sum);

		point_added = 1;
		time_last_pose_was_added = pose_and_time.second;
	}

	//if (smooth_trajectory.size() < 2)
	if (trajectory.size() < 2)
		return vector<carmen_ackerman_traj_point_t>();

	// remove points behind car, etc.
	vector<carmen_ackerman_traj_point_t> filtered_trajectory = filter_trajectory(
			//smooth_trajectory, pose_and_time.first);
			trajectory, pose_and_time.first);

// Print to experiment data	
/*	printf("TARGET_POSE_FILTERED x: %lf y: %lf th: %lf v: %lf timestamp: %lf\n",
			filtered_trajectory[filtered_trajectory.size() - 1].x,
			filtered_trajectory[filtered_trajectory.size() - 1].y,
			filtered_trajectory[filtered_trajectory.size() - 1].theta,
			filtered_trajectory[filtered_trajectory.size() - 1].v,
			image_timestamp);
*/

	if(filtered_trajectory.size() > 2)
		correct_thetas(filtered_trajectory);

	if (point_added)
	{
		//To run with smoother
		//		if (target_poses.size() > 1)
		//		{
		//			poses_smooth = path_smoother.Smooth(target_poses_new);
		//		}
		//
		//		// CHECAR POR QUE CHEGOU COM 0 AQUI
		//		if (poses_smooth.size() > 0)
		//			poses_smooth.pop_back();

		//plot_to_debug_state(filtered_trajectory, target_pose_in_the_world, pose_and_time.first, MAX_POSES_IN_TRAJECTORY);
	}

	return filtered_trajectory;
}



void
publish_trajectory(vector<carmen_ackerman_traj_point_t> poses_complete,double timestamp)
{
	if (poses_complete.size() > 0)
	{
		int annotations[1000];
		IPC_RETURN_TYPE err;
		carmen_rddf_road_profile_message path_planner_road_profile_message;

		path_planner_road_profile_message.poses = &poses_complete[0];
		path_planner_road_profile_message.poses_back = 0;
		path_planner_road_profile_message.number_of_poses = poses_complete.size();
		path_planner_road_profile_message.number_of_poses_back = 0;
		path_planner_road_profile_message.annotations = annotations;
		path_planner_road_profile_message.timestamp = timestamp;
		path_planner_road_profile_message.host = carmen_get_host();

		err = IPC_publishData(CARMEN_RDDF_ROAD_PROFILE_MESSAGE_NAME, &path_planner_road_profile_message);
		carmen_test_ipc_exit(err, "Could not publish", CARMEN_RDDF_ROAD_PROFILE_MESSAGE_FMT);
	}
}

/*

static vector<carmen_ackerman_traj_point_t>
publishSplineRDDF()
{
	static std::vector<double> X;
	static std::vector<double> Y;
	static std::vector<double> I;
	static vector<double> pose_times;
	static vector<double> pose_thetas;
	static unsigned int maxPositions = 20;
	static unsigned int extraPositions = 0;

	vector<carmen_ackerman_traj_point_t> poses;
	tk::spline x, y;
	carmen_ackerman_traj_point_t localizePose;
	carmen_vector_3D_t target_pose_in_the_world;

//	static int skipSplinePoints = 0;
//	if (skipSplinePoints < 5)
//	{
//		skipSplinePoints++;
//		return;
//	}
//	skipSplinePoints = 0;

	double minTimestampDiff = DBL_MAX;
	int minTimestampIndex = -1;

	for (unsigned int i = 0; i < localizeVector.size(); i++)
	{
		if(fabs(localizeVector[i].timestamp - velodyne_message_arrange->timestamp) < minTimestampDiff)
		{
			minTimestampIndex = i;
			minTimestampDiff = fabs(localizeVector[i].timestamp - velodyne_message_arrange->timestamp);
		}
	}

//	printf("LOCALIZE FOUND: %d DIFF: %lf POSE %lf %lf TIME: %lf\n", minTimestampIndex, minTimestampDiff, localizeVector[minTimestampIndex].globalpos.x,
//			localizeVector[minTimestampIndex].globalpos.y, localizeVector[minTimestampIndex].timestamp);


	if(localizeVector.size() < 1)
		return poses;

	localizePose.x = localizeVector[minTimestampIndex].globalpos.x;
	localizePose.y = localizeVector[minTimestampIndex].globalpos.y;
	localizePose.theta = localizeVector[minTimestampIndex].globalpos.theta;
	localizePose.v = localizeVector[minTimestampIndex].v;
	localizePose.phi = localizeVector[minTimestampIndex].phi;


	target_pose_in_the_world = move_point_from_velodyne_frame_to_world_frame(trackerPoint, localizePose);

	// *********************************************************************************
	// double distancia = sqrt(pow(target_pose_in_the_world.x - localizePose.x, 2) +
	// 		pow(target_pose_in_the_world.y - localizePose.y, 2));
    //
	// printf("distancia: %lf time diff: %lf\n", distancia, minTimestampDiff);
    //
	// double sph_r = sqrt(trackerPoint.x * trackerPoint.x + trackerPoint.y * trackerPoint.y +
	// 		trackerPoint.z * trackerPoint.z);
	// double sph_v = asin(trackerPoint.z / sph_r);
	// double sph_h = atan2(trackerPoint.y, trackerPoint.x);
    //
	// carmen_sphere_coord_t sphere_point;
	// sphere_point.horizontal_angle = sph_h;
	// sphere_point.vertical_angle = sph_v;
	// sphere_point.length = sph_r - distancia;
    //
	// trackerPoint = carmen_covert_sphere_to_cartesian_coord(sphere_point);
    //
	// target_pose_in_the_world = move_point_from_velodyne_frame_to_world_frame(trackerPoint, localizePose);
	// *********************************************************************************

	//printf("LOCALIZE: %lf %lf TARGET: %lf %lf\n",
			//localizeVector[minTimestampIndex].globalpos.x,
			//localizeVector[minTimestampIndex].globalpos.y,
			//target_pose_in_the_world.x,
			//target_pose_in_the_world.y);

//	printf("publisher sic: X: %lf Y: %lf\n", localizePose.x, localizePose.y);

//	double distancia = sqrt(pow(trackerPoint.x, 2) + pow(trackerPoint.y, 2));
//	double angulo = atan2(trackerPoint.y, trackerPoint.x);
	//distancia -= 4; //TODO porque?

//	X.push_back(localizePose.x + distancia * cos(carmen_normalize_theta(localizePose.theta + carmen_degrees_to_radians(angulo))));
//	Y.push_back(localizePose.y + distancia * sin(carmen_normalize_theta(localizePose.theta + carmen_degrees_to_radians(angulo))));
//	I.push_back(index++);

//	X.push_back(localizePose.x + tracker_in_car_reference.x * cos(localizePose.theta) - tracker_in_car_reference.y * sin(localizePose.theta));
//	Y.push_back(localizePose.y + tracker_in_car_reference.x * sin(localizePose.theta) + tracker_in_car_reference.y * cos(localizePose.theta));
//	I.push_back(index++);

	if ((X.size() == 0) || (X[X.size() - 1] != target_pose_in_the_world.x))
	{
		pose_times.push_back(localizeVector[minTimestampIndex].timestamp);
		pose_thetas.push_back(localizePose.theta);

		X.push_back(target_pose_in_the_world.x);
		Y.push_back(target_pose_in_the_world.y);

		// X.push_back(localizePose.x + tracker_in_car_reference.x * cos(localizePose.theta) - tracker_in_car_reference.y * sin(localizePose.theta));
		// Y.push_back(localizePose.y + tracker_in_car_reference.x * sin(localizePose.theta) + tracker_in_car_reference.y * cos(localizePose.theta));

		// X.push_back(localizePose.x);
		// Y.push_back(localizePose.y);
	}

	//I.push_back(index++);

	if (I.size() > maxPositions)
	{
		X.erase(X.begin());
		Y.erase(Y.begin());
		//I.erase(I.begin());
		pose_times.erase(pose_times.begin());
		pose_thetas.erase(pose_thetas.begin());
	}

	I.clear();
	for (unsigned int i = 0; i < X.size(); i++)
		I.push_back(i);

//	printf("\n\n-----------------------\n");

	std::vector<double> Xspline;
	std::vector<double> Yspline;
	std::vector<double> Ispline;

	std::vector<double> Xteste;
	std::vector<double> Yteste;
	std::vector<double> Tteste;

	double first_pose_x = localizeVector[localizeVector.size() - 1].globalpos.x;
	double first_pose_y = localizeVector[localizeVector.size() - 1].globalpos.y;
	double first_pose_theta = localizeVector[localizeVector.size() - 1].globalpos.theta;

	Xspline.push_back(localizeVector[localizeVector.size() - 1].globalpos.x - first_pose_x);
	Yspline.push_back(localizeVector[localizeVector.size() - 1].globalpos.y - first_pose_y);
	Ispline.push_back(0);

	Xteste.push_back(0.0);
	Yteste.push_back(0.0);
	Tteste.push_back(0.0);

//	double target_x = 0.0;
//	double target_y = 0.0;

	//double angle = 0.0;
	//double angle_diff = 0.0;

	g2o::SE2 robot_pose(first_pose_x, first_pose_y, first_pose_theta);
	g2o::SE2 last_pose(first_pose_x, first_pose_y, first_pose_theta);

	int n = 1;

	for (unsigned int i = 0; i < X.size(); i++)
	{
//		double pix = X[i] - first_pose_x;
//		double piy = Y[i] - first_pose_y;

		g2o::SE2 target_in_world_reference(X[i], Y[i], 0);
		g2o::SE2 target_in_last_pose_reference = last_pose.inverse() * target_in_world_reference;
		g2o::SE2 target_in_car_reference = robot_pose.inverse() * target_in_world_reference;

//		target_x = target_in_car_reference[0];
//		target_y = target_in_car_reference[1];
//		angle = fabs(carmen_radians_to_degrees(atan2(piy - Yspline.back(), pix - Xspline.back())));

		double angle_in_the_world = atan2(Y[i] - last_pose[1], X[i] - last_pose[0]);
		double angle_in_car_reference = atan2(target_in_car_reference[1], target_in_car_reference[0]);
		double angle_from_last_pose = atan2(target_in_last_pose_reference[1], target_in_last_pose_reference[0]);

		//if(Tteste.size() > 2)
			//angle_diff = fabs(carmen_radians_to_degrees(carmen_normalize_theta(mrpt::math::angDistance(angle, Tteste.back()))));

		//printf("X: %lf  Y: %lf angle: %lf\n",target_x, target_y, angle);
		//printf("Xback: %lf  Yback: %lf angleDiff: %lf size: %d\n\n", Xteste.back(), Yteste.back(), angle_diff, Tteste.size());

//		double dt = (pose_times[i] - pose_times[i - 1]);
//		double dist = sqrt(pow(piy - pi_1y, 2) + pow(pix - pi_1x, 2));

		if ((fabs(carmen_radians_to_degrees(angle_from_last_pose)) > 20.0) || (angle_diff > 20.0)
				|| (target_in_last_pose_reference[0] < 0)
				|| target_in_car_reference[0] < 4.5){
			X.erase(X.begin() + i);
			Y.erase(Y.begin() + i);
			continue;
		}

//		Xspline.push_back(pix);
//		Yspline.push_back(piy);
		Ispline.push_back(n++);

		Xteste.push_back(target_in_car_reference[0]);
		Yteste.push_back(target_in_car_reference[1]);
		Tteste.push_back(angle_in_car_reference); // TODO: CORRIGIR ESSE ANGULO. ELE ESTA NA REF. DO PONTO ANTERIOR, MAS ELE DEVERIA ESTAR NA REF. DO CARRO.

		last_pose = g2o::SE2(X[i], Y[i], angle_in_the_world);
		printf("\tX: %lf Y: %lf T: %lf\n", target_in_car_reference[0], target_in_car_reference[1], angle_in_car_reference);
	}

	printf("Xteste size: %ld\n", Xteste.size());

	//getchar();

	if (Ispline.size() > 1)
	{
		x.set_points(Ispline, Xteste Xspline, true);
		y.set_points(Ispline, Yteste Yspline, true);
		carmen_ackerman_traj_point_t newPose;

		// TODO: MOVER OS PONTOS DA SPLINE PARA A REFERENCIA DO MUNDO.
		g2o::SE2 spline_point_in_car_reference(x(Ispline[0]), y(Ispline[0]), 0);
		g2o::SE2 spline_point_in_the_world = robot_pose * spline_point_in_car_reference;

		newPose.x = spline_point_in_the_world[0];
		newPose.y = spline_point_in_the_world[1];
		//newPose.x = x(Ispline[0]) + first_pose_x;
		//newPose.y = y(Ispline[0]) + first_pose_y;

		poses.push_back(newPose);

		for (unsigned int i = 1; i < Ispline.size() + (unsigned int) extraPositions; i++)
		{
			// TODO: MOVER OS PONTOS DA SPLINE PARA A REFERENCIA DO MUNDO.
			g2o::SE2 spline_point_in_car_reference(x(Ispline[0] + i), y(Ispline[0] + i), 0);
			g2o::SE2 spline_point_in_the_world = robot_pose * spline_point_in_car_reference;

			newPose.x = spline_point_in_the_world[0];
			newPose.y = spline_point_in_the_world[1];
			//newPose.x = x(Ispline[0] + i) + first_pose_x;
			//newPose.y = y(Ispline[0] + i) + first_pose_y;
			newPose.v = 1.0;

			poses.push_back(newPose);

			double piy = poses[i].y - poses[0].y;
			double pi_1y = poses[i - 1].y - poses[0].y;
			double pix = poses[i].x - poses[0].x;
			double pi_1x = poses[i - 1].x - poses[0].x;

			poses[i - 1].theta = atan2(piy - pi_1y, pix - pi_1x);

			double dt = (pose_times[i] - pose_times[i - 1]);
			double dist = sqrt(pow(piy - pi_1y, 2) + pow(pix - pi_1x, 2));
			double predv = dist / dt;
			predv++;



//			printf("PX: %lf PY: %lf P-1X: %lf P-1Y: %lf DIST: %lf\n", pix, piy, pi_1x, pi_1y, dist);
//			printf("TEST: %lf TLOC:%lf TDIFF: %lf v: %lf vpred: %lf dt: %lf ID: %lf\n", poses[i - 1].theta, pose_thetas[i - 1],
//					fabs(poses[i - 1].theta - pose_thetas[i - 1]),
//					localizePose.v, predv, dt, Ispline[0] + i);

		}
	}

	// CHECAR POR QUE CHEGOU COM 0 AQUI
	if (poses.size() > 0)
		poses.pop_back();

	//for (unsigned int i = 0; i < poses.size(); i++)
		//printf("X: %lf Y: %lf TH: %lf\n", poses[i].x, poses[i].y, poses[i].theta);
	//printf("\n\n-----------------------\n");

//	for (unsigned int i = 0; i < poses.size(); i++)
//		printf("X: %lf Y: %lf TH: %lf\n", poses[i].x, poses[i].y, poses[i].theta);
//	printf("\n\n-----------------------\n");

	plot_to_debug_state(poses, target_pose_in_the_world, localizePose, 100);

	return poses;
}
 */

///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Handlers                                                                                  //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


static void
localize_globalpos_handler(carmen_localize_ackerman_globalpos_message *message)
{
	//printf("LOCALIZER: %lf %lf\n", message->globalpos.x, message->globalpos.y);


	//TODO primeira pose do localize no playback = 0.0 ERROOO!!!
	if(fabs(message->globalpos.x) < 0.00001  || fabs(message->globalpos.y) < 0.00001 || fabs(message->timestamp) < 0.0001){
		return;
	}

	//print to experiment data
/*	printf("LOCALIZER x: %lf y: %lf th: %lf time: %lf v: %lf phi: %lf\n",
			message->globalpos.x,
			message->globalpos.y,
			message->globalpos.theta,
			message->timestamp,
			message->v,
			message->phi);
*/

	if(first_matrix){
		car_to_global_matrix = create_rotation_matrix(message->pose.orientation);
		first_matrix = 0;
	}

	localizeVector.push_back(*message);

	if (localizeVector.size() > maxPositions)
	{
		localizeVector.erase(localizeVector.begin());
	}
}


void
compute_fps(carmen_bumblebee_basic_stereoimage_message* image_msg)
{
	static double last_timestamp = 0.0;
	static double last_time = 0.0;
	double time_now = carmen_get_time();

	if (!received_image)
	{
		received_image = 1;
		last_timestamp = image_msg->timestamp;
		last_time = time_now;
	}

	if ((image_msg->timestamp - last_timestamp) > 1.0)
	{
		msg_last_fps = msg_fps;
		msg_fps = 0;
		last_timestamp = image_msg->timestamp;
	}
	msg_fps++;

	if ((time_now - last_time) > 1.0)
	{

		disp_last_fps = disp_fps;
		disp_fps = 0;
		last_time = time_now;
	}
	disp_fps++;
}


void
mouseHandler(int event, int x, int y, int flags, void *param)
{
//	if (!mark_bbox_active)
//		return;
	Mat *img = (Mat *) param;

	if (x < 0 || y < 0 || x > img->cols || y > img->rows)
		return;

	static int drag = 0;
	static cv::Point clicked_point(0, 0);

	// just to avoid the unused warnings
	(void) flags;
	(void) param;

	/* user press left button */
	if(event == CV_EVENT_LBUTTONDOWN && !drag)
	{
		clicked_point = cv::Point(x, y);
		printf("Clicked point: %d %d\n", x, y);
		drag = 1;
	}

	/* user drag the mouse */
	if(event == CV_EVENT_MOUSEMOVE && drag)
	{
		printf("Drawing rect\n");
		image_temp_bbox = Rect(clicked_point.x, clicked_point.y, x - clicked_point.x, y - clicked_point.y);
	}

	/* user release left button */
	if(event == CV_EVENT_LBUTTONUP && drag)
	{
		printf("BBox set: %d %d %d %d\n", clicked_point.x, clicked_point.y, x - clicked_point.x, y - clicked_point.y);
		image_final_bbox = Rect(clicked_point.x, clicked_point.y, x - clicked_point.x, y - clicked_point.y);
		image_temp_bbox = Rect(-1, -1, -1, -1);
		show_final_image = 1;
		drag = 0;
	}
}


void
write_additional_information(Mat *m, double timestamp)
{
	char string1[1024];
	strcpy(string1, "");
	CvFont font;

	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, .4, .5, 0, 1, 8);
	sprintf(string1, "FPS:%d X: %.2lf Y: %.2lf RD: %.2lf LD: %.3lf TP: %d pL: %d pR: %d pF: %d po: %d",
			disp_last_fps, trackerPoint.x , trackerPoint.y, /*trackerPoint.z,*/
			avg_range_diff, lat_shift, quant_points_in_box, quant_points_in_box_left, quant_points_in_box_right, quant_points_in_box_free, quant_points_in_box_obstacle );

	cv::Size s = m->size();
	cv::Point textOrg(25, 25);

	cv::rectangle(*m, cv::Point(0, 0), cv::Point(s.width, 50), Scalar::all(0), -1);
	cv::putText(*m, string1, textOrg, FONT_HERSHEY_SIMPLEX, 0.4, Scalar::all(255), 1, 8);
}


void
handle_char(Mat *preprocessed_image, char c)
{
	if (c == '\r' || c == '\n')
	{
		tracker_restart_requested = 1;

		box.x1_ = image_final_bbox.x / image_zoom;
		box.y1_ = image_final_bbox.y / image_zoom;
		box.x2_ = (image_final_bbox.x + image_final_bbox.width) / image_zoom;
		box.y2_ = (image_final_bbox.y + image_final_bbox.height) / image_zoom;

		marked_width = image_final_bbox.width / image_zoom;
		marked_height = image_final_bbox.height / image_zoom;

		tracker.Init(*preprocessed_image, box, &regressor);
		//carmen_voice_send_alert((char *) "Ah!\n");
		tracker_is_initialized = 1;
		retain_image_active = 0;
		show_final_image = 0;
	}
	else if (c == 'p')
		retain_image_active = !retain_image_active;
	else if (c == 'v')
		show_velodyne_active = !show_velodyne_active;
	else if (c == 'a')
		display_all_points_velodyne = !display_all_points_velodyne;
}


void
display_interfaces(carmen_bumblebee_basic_stereoimage_message* image_msg, Mat *preprocessed_image)
{
	static Mat *mini_img = NULL;
	static Mat *view_img = NULL;

	if (mini_img == NULL)
	{
		mini_img = new Mat(preprocessed_image->rows * image_zoom, preprocessed_image->cols * image_zoom, preprocessed_image->type());
		view_img = new Mat(preprocessed_image->rows * image_zoom, preprocessed_image->cols * image_zoom, preprocessed_image->type());
		resize(*preprocessed_image, *mini_img, mini_img->size());
	}

	if (!retain_image_active)
		resize(*preprocessed_image, *mini_img, mini_img->size());

	mini_img->copyTo(*view_img);

	if (image_temp_bbox.x > 0 && image_temp_bbox.width > 0)
		rectangle(*view_img, image_temp_bbox, Scalar(255, 0, 0), 1);

	if (show_final_image && image_final_bbox.x > 0 && image_final_bbox.width > 0)
		rectangle(*view_img, image_final_bbox, Scalar(0, 255, 0), 1);

	if (box.x1_ > 0 && box.x2_ > 0)
	{
		Rect r(box.x1_ * image_zoom, box.y1_ * image_zoom, (box.x2_ - box.x1_) * image_zoom, (box.y2_ - box.y1_) * image_zoom);
		rectangle(*view_img, r, Scalar(0, 0, 255), 2);
	}

	compute_fps(image_msg);
	write_additional_information(view_img, image_msg->timestamp);

	imshow("bumblebee", *view_img);
	cv::setMouseCallback("bumblebee", mouseHandler, view_img);
	setlocale(LC_ALL, "C");
	char c = waitKey(5);

	handle_char(preprocessed_image, c);
}


int
box_is_valid()
{
	if (box_1.x < 0.0 || box_1.y < 0.0) return 0;
	else if (box_1.height < 0.0 || box_1.width < 0.0) return 0;
	else return 1;
}


double
compute_average_range_diff(deque<carmen_vector_3D_t> last_target_poses_in_laser)
{
	if (last_target_poses_in_laser.size() <= 1)
		return 0;

	double range = 0;

	for (uint i = 1; i < last_target_poses_in_laser.size(); i++)
	{
		//printf("Range[%d]: %lf diff: %lf\n", i, last_target_poses_in_laser[i].x, fabs(last_target_poses_in_laser[i].x - last_target_poses_in_laser[i - 1].x));
		range += fabs(last_target_poses_in_laser[i].x - last_target_poses_in_laser[i - 1].x);
	}

	range /= (double) last_target_poses_in_laser.size();
	return range;
}


double
compute_average_lateral_shift(deque<carmen_vector_3D_t> last_target_poses_in_laser)
{
	if (last_target_poses_in_laser.size() <= 1)
		return 0;

	double lateral_motion = 0;

	for (uint i = 1; i < last_target_poses_in_laser.size(); i++)
	{
		//printf("Y[%d]: %lf diff: %lf\n", i, last_target_poses_in_laser[i].y, fabs(last_target_poses_in_laser[i].y - last_target_poses_in_laser[i - 1].y));
		lateral_motion += fabs(last_target_poses_in_laser[i].y - last_target_poses_in_laser[i - 1].y);
	}

	lateral_motion /= (double) last_target_poses_in_laser.size();
	return lateral_motion;
}


int
target_is_valid()
{
	static deque<carmen_vector_3D_t> last_target_poses_in_laser;

	avg_range_diff = compute_average_range_diff(last_target_poses_in_laser);
	lat_shift = compute_average_lateral_shift(last_target_poses_in_laser);

	printf("Range variation: %lf range: %lf \t Lateral shift: %lf!\n", avg_range_diff, trackerPoint.x, fabs(lat_shift));

	if (tracker_restart_requested)
		last_target_poses_in_laser.clear();

	last_target_poses_in_laser.push_back(trackerPoint);

	if (last_target_poses_in_laser.size() > 10)
		last_target_poses_in_laser.pop_front();

	if (last_target_poses_in_laser.size() <= 5 ||
		((avg_range_diff < 1.0) && (lat_shift < 0.5)))
	{
		return 1;
	}

	if (avg_range_diff > 5.0) printf("Invalid range variation: %lf range: %lf!\n", avg_range_diff, trackerPoint.x);
	if (lat_shift > 5.0) printf("Invalid lateral shift: %lf!\n", fabs(lat_shift));

	return 0;
}


int
errors_exist_in_tracking(int success, double timestamp)
{
	int error_detected = 0;
	static double time_since_box_became_valid = timestamp;

	if (tracker_restart_requested)
		time_since_box_became_valid = timestamp;

	if (tracker_is_initialized && (!success || !box_is_valid())) // what to do???
	{
		printf("sucess: %d box_is_valid(): %d\n", success, box_is_valid());
		error_detected = 1;
	}
	else if (tracker_is_initialized && !target_is_valid())
		error_detected = 1;
	else
	{
		time_since_box_became_valid = timestamp;
		return 0;
	}

	if (error_detected && (fabs(timestamp - time_since_box_became_valid) > 0.5))
	{
		// give control to the user
		//carmen_voice_send_alert("Sistema de rastreamento instavel! Por favor, assuma o controle!");
	}

	return 1;
}


int
errors_exist_trajectory_generation(vector<carmen_ackerman_traj_point_t> trajectory, double timestamp)
{
	int error_detected = 0;
	static double time_since_trajectory_became_valid = timestamp;

	if (tracker_restart_requested)
		time_since_trajectory_became_valid = timestamp;

	if (tracker_is_initialized && trajectory.size() <= 0) // what to do???
	{
		printf("trajectory.size(): %ld\n", trajectory.size());
		error_detected = 1;
	}
	else
	{
		time_since_trajectory_became_valid = timestamp;
		return 0;
	}

	if (error_detected)
	{
		if (fabs(timestamp - time_since_trajectory_became_valid) > 0.5)
		{
			// give control to the user
			//carmen_voice_send_alert("Geracao de trajetorias instavel! Por favor, assuma o controle!");
		}
	}

	return 1;
}


static void
image_handler(carmen_bumblebee_basic_stereoimage_message* image_msg)
{
	if (exit_requested)
		exit(printf("Closing...\n"));

	if (image_msg->isRectified)
	{
		vector<carmen_ackerman_traj_point_t> trajectory;
		Mat preprocessed_image = image_pre_processing(image_msg);
		carmen_velodyne_partial_scan_message velodyne_sync_with_cam;

		int success = call_tracker_and_compute_object_3d_position(
				&preprocessed_image, image_msg->height, image_msg->width,
				image_msg->timestamp, &velodyne_sync_with_cam);

		display_interfaces(image_msg, &preprocessed_image);

		double marked_h_w_ratio = (double) marked_width / (double) marked_height;
		double measured_h_w_ratio = (double) fabs(box.x2_ - box.x1_) / (double) fabs(box.y2_ - box.y1_);

		if (measured_h_w_ratio / marked_h_w_ratio > 2.0)
		{
			Mat img(300, 300, CV_8UC3);
			rectangle(img, Rect(0,0,300,300), Scalar(0, 0, 255), -1);
			imshow("status", img);
		}
		else
		{
			Mat img(300, 300, CV_8UC3);
			rectangle(img, Rect(0,0,300,300), Scalar(0, 255, 0), -1);
			imshow("status", img);
		}

		if (!success || errors_exist_in_tracking(success, image_msg->timestamp))
		{
			Mat img(300, 300, CV_8UC3);
			rectangle(img, Rect(0,0,300,300), Scalar(0, 0, 255), -1);
			imshow("status", img);

			return;
		}

		if (tracker_is_initialized)
		{
			trajectory = build_trajectory(image_msg->timestamp, &velodyne_sync_with_cam);

			if (errors_exist_trajectory_generation(trajectory, image_msg->timestamp))
			{
				Mat img(300, 300, CV_8UC3);
				rectangle(img, Rect(0,0,300,300), Scalar(0, 0, 255), -1);
				imshow("status", img);
				return;
			}

			publish_box_message(image_msg->host, image_msg->timestamp);
			publish_trajectory(trajectory, image_msg->timestamp);

			if (tracker_restart_requested)
				tracker_restart_requested = 0;
		}

	}
}


void
velodyne_partial_scan_message_handler(carmen_velodyne_partial_scan_message *velodyne_message)
{
	//	static int n = 0;
	//	static double time = 0;
	//
	//	if (velodyne_message->timestamp - time > 1.0)
	//	{
	//		printf("VELO: %d\n", n);
	//		time = velodyne_message->timestamp;
	//		n = 0;
	//	}
	//	else
	//		n++;
	velodyne_message_arrange = velodyne_message;
	carmen_velodyne_camera_calibration_arrange_velodyne_vertical_angles_to_true_position(velodyne_message_arrange);

	carmen_velodyne_partial_scan_message velodyne_copy;

	velodyne_copy.host = velodyne_message_arrange->host;
	velodyne_copy.number_of_32_laser_shots = velodyne_message_arrange->number_of_32_laser_shots;
	velodyne_copy.partial_scan = (carmen_velodyne_32_laser_shot*)malloc(sizeof(carmen_velodyne_32_laser_shot) * velodyne_message_arrange->number_of_32_laser_shots);
	memcpy(velodyne_copy.partial_scan, velodyne_message_arrange->partial_scan, sizeof(carmen_velodyne_32_laser_shot) * velodyne_message_arrange->number_of_32_laser_shots);
	velodyne_copy.timestamp = velodyne_message_arrange->timestamp;

	velodyne_vector.push_back(velodyne_copy);

		if (velodyne_vector.size() > maxPositions)
		{
			free(velodyne_vector.begin()->partial_scan);
			velodyne_vector.erase(velodyne_vector.begin());
		}
	//	show_velodyne(velodyne_message);
}


static void
carmen_obstacle_distance_mapper_message_handler(carmen_obstacle_distance_mapper_map_message *message)
{
	path_smoother.distance_map = message;
}

///////////////////////////////////////////////////////////////////////////////////////////////

static void
shutdown_camera_view(int x)
{
	if (x == SIGINT)
	{
//		carmen_ipc_disconnect();
		printf("Disconnected from robot.\n");
		exit_requested = 1;
		exit(0);
	}
}

//
static int
read_parameters(int argc, char **argv, int camera)
{
	int num_items;
	std::ostringstream myStream;
	myStream << camera << std::flush;
	std::string camera_string = "camera";
	camera_string.append(myStream.str());

	carmen_param_t param_list[] = {
			{(char*) "tracker_opentld", 	(char*) "view_width", CARMEN_PARAM_INT, &tld_image_width, 0, NULL},
			{(char*) "tracker_opentld", 	(char*) "view_height", CARMEN_PARAM_INT, &tld_image_height, 0, NULL},
			{(char*) camera_string.c_str(), (char*) "x", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.position.x), 0, NULL},
			{(char*) camera_string.c_str(), (char*) "y", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.position.y), 0, NULL},
			{(char*) camera_string.c_str(), (char*) "z", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.position.z), 0, NULL},
			{(char*) camera_string.c_str(), (char*) "roll", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.orientation.roll), 0, NULL},
			{(char*) camera_string.c_str(), (char*) "pitch", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.orientation.pitch), 0, NULL},
			{(char*) camera_string.c_str(), (char*) "yaw", CARMEN_PARAM_DOUBLE, &(camera_pose_parameters.orientation.yaw), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "x", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.position.x), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "y", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.position.y), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "z", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.position.z), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "roll", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.orientation.roll), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "pitch", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.orientation.pitch), 0, NULL},
			{(char*) "sensor_board_1",      (char*) "yaw", CARMEN_PARAM_DOUBLE, &(board_pose_parameters.orientation.yaw), 0, NULL},
			{(char *) "velodyne",    (char *) "x", 	CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.position.x), 0, NULL},
			{(char *) "velodyne",    (char *) "y", 	CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.position.y), 0, NULL},
			{(char *) "velodyne",    (char *) "z", 	CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.position.z), 0, NULL},
			{(char *) "velodyne",    (char *) "yaw", 	CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.orientation.yaw), 0, NULL},
			{(char *) "velodyne",    (char *) "pitch", CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.orientation.pitch), 0, NULL},
			{(char *) "velodyne",    (char *) "roll",  CARMEN_PARAM_DOUBLE, &(velodyne_pose_parameters.orientation.roll), 0, NULL},
			{(char *) "robot",	(char *) "length",								  		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.length,								 		1, NULL},
			{(char *) "robot",	(char *) "width",								  		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.width,								 			1, NULL},
			{(char *) "robot", 	(char *) "distance_between_rear_wheels",		  		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.distance_between_rear_wheels,			 		1, NULL},
			{(char *) "robot", 	(char *) "distance_between_front_and_rear_axles", 		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.distance_between_front_and_rear_axles, 		1, NULL},
			{(char *) "robot", 	(char *) "distance_between_front_car_and_front_wheels",	CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.distance_between_front_car_and_front_wheels,	1, NULL},
			{(char *) "robot", 	(char *) "distance_between_rear_car_and_rear_wheels",	CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.distance_between_rear_car_and_rear_wheels,		1, NULL},
			{(char *) "robot", 	(char *) "max_velocity",						  		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.max_v,									 		1, NULL},
			{(char *) "robot", 	(char *) "max_steering_angle",					  		CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.max_phi,								 		1, NULL},
			{(char *) "robot", 	(char *) "maximum_acceleration_forward",				CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.maximum_acceleration_forward,					1, NULL},
			{(char *) "robot", 	(char *) "maximum_acceleration_reverse",				CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.maximum_acceleration_reverse,					1, NULL},
			{(char *) "robot", 	(char *) "maximum_deceleration_forward",				CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.maximum_deceleration_forward,					1, NULL},
			{(char *) "robot", 	(char *) "maximum_deceleration_reverse",				CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.maximum_deceleration_reverse,					1, NULL},
			{(char *) "robot", 	(char *) "maximum_steering_command_rate",				CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.maximum_steering_command_rate,					1, NULL},
			{(char *) "robot", 	(char *) "understeer_coeficient",						CARMEN_PARAM_DOUBLE, &path_smoother.robot_config.understeer_coeficient,							1, NULL},

	};
	sensor_board_to_car_matrix = create_rotation_matrix(board_pose_parameters.orientation);
	sensor_to_board_matrix = create_rotation_matrix(velodyne_pose.orientation);

	num_items = sizeof (param_list) / sizeof (param_list[0]);
	carmen_param_install_params(argc, argv, param_list, num_items);

	return 0;
}

int
main(int argc, char **argv)
{
	int camera = 0;

	if (argc != 3)
	{
		fprintf(stderr, "%s: Wrong number of parameters. tracker_opentld requires 2 parameter and received %d. \n Usage: %s <camera_number> <camera_side(0-left; 1-right)\n>", argv[0], argc - 1, argv[0]);
		exit(1);
	}

	setlocale(LC_ALL, "C");

	camera = atoi(argv[1]);
	camera_side = atoi(argv[2]);

	carmen_ipc_initialize(argc, argv);
	carmen_param_check_version(argv[0]);
	read_parameters(argc, argv, camera);
	last_track.resize(num_prev_frames);
	camera_parameters = get_stereo_instance(camera, tld_image_width, tld_image_height);

	box.x1_ = -1.0;
	box.y1_ = -1.0;
	box.x2_ = -1.0;
	box.y2_ = -1.0;

	signal(SIGINT, shutdown_camera_view);

	carmen_visual_tracker_define_messages();
	carmen_bumblebee_basic_subscribe_stereoimage(camera, NULL, (carmen_handler_t) image_handler, CARMEN_SUBSCRIBE_LATEST);
	carmen_velodyne_subscribe_partial_scan_message(NULL,(carmen_handler_t)velodyne_partial_scan_message_handler, CARMEN_SUBSCRIBE_LATEST);
	carmen_localize_ackerman_subscribe_globalpos_message(NULL, (carmen_handler_t) localize_globalpos_handler, CARMEN_SUBSCRIBE_LATEST);
	carmen_obstacle_distance_mapper_subscribe_message(NULL,	(carmen_handler_t) carmen_obstacle_distance_mapper_message_handler, CARMEN_SUBSCRIBE_LATEST);

	carmen_ipc_dispatch();

}
