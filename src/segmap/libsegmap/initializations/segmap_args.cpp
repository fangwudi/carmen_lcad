
#include <cstdlib>
#include <string>
#include <carmen/segmap_args.h>
#include <carmen/command_line.h>

using namespace std;


string
default_data_dir()
{
	return string(getenv("CARMEN_HOME")) + "/src/segmap/data/";
}


void
add_default_slam_args(CommandLineArguments &args)
{
	args.add_positional<string>("log_path", "Path of a log", 1);
	args.add<double>("v_thresh", "Skip data packages with absolute velocity below this theshold", 0);
	args.add<int>("step,s", "Number of data packages to skip", 1);
}


void
add_default_mapper_args(CommandLineArguments &args)
{
	// map parameters
	args.add<double>("resolution,r", "Map resolution", 0.2);
	args.add<double>("tile_size,t", "Map tiles size", 50);
	args.add<string>("map_path,m", "Path to save the maps", "/tmp/map");

	args.save_config_file(default_data_dir() + "/mapper_config.txt");
}


void
add_default_localizer_args(CommandLineArguments &args)
{
	// localization parameters
	args.add<int>("n_particles", "Number of particles", 30);
	args.add<int>("use_gps_weight", "Flag to choose if GPS data should be used for weighting particles", 0);
	args.add<int>("use_map_weight", "Flag to choose if the map should be used for weighting particles", 1);
	args.add<double>("gps_xy_std", "Std of gps position (m)", 0.5);
	args.add<double>("gps_h_std", "Std of gps heading estimates (degrees)", 5);
	args.add<double>("v_std", "Std of linear velocity measurements (m/s)", 0.2);
	args.add<double>("phi_std", "Std of steering wheel angle measurements (degrees)", 0.5);
	args.add<double>("odom_xy_std", "Std of dead reckoning position estimates (m)", 0.1);
	args.add<double>("odom_h_std", "Std of dead reckoning heading estimates (degrees)", 0.5);
	args.add<double>("color_red_std", "Std of pixel color measurements", 10.);
	args.add<double>("color_green_std", "Std of pixel color measurements", 10.);
	args.add<double>("color_blue_std", "Std of pixel color measurements", 10.);
	args.add<int>("seed", "Seed for pseudo-random number generator", 0);
	args.add<int>("correction_step", "Frequency in which correction takes place [<= 1 for always correcting]", 1);
	args.add<int>("steps_to_skip_map_reload", "Minimum number of steps to wait until a new map reload from disk", 5);

	args.save_config_file(default_data_dir() + "/localizer_config.txt");
}


void
add_default_sensor_preproc_args(CommandLineArguments &args)
{
	args.add<double>("ignore_above_threshold", "Ignore points with z-coord (in sensor reference) above this threshold", DBL_MAX);
	args.add<double>("ignore_below_threshold", "Ignore points with z-coord (in sensor reference) below this threshold", -DBL_MAX);
	args.add<double>("offset_x", "Offset to subtract the pose (x-coord)", 7757888.199148);
	args.add<double>("offset_y", "Offset to subtract the pose (y-coord)", -363560.975411);
	args.add<int>("use_xsens,x", "Whether or not to use pitch, and roll angles from xsens", 1);
	args.add<int>("gps_id", "Id of the gps to be used", 1);
	args.add<string>("intensity_mode,i", "What type of information to assign to LiDAR rays intensity [remission | visual | semantic | raw]", "remission");
	args.add<int>("use_intensity_calibration", "Flag to choose using the velodyne calibration or not.", 0);

	args.save_config_file(default_data_dir() + "/preproc_config.txt");
}

