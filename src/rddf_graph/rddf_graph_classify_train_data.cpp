#include "rddf_graph_utils.h"

using namespace std;
bool g_ipc_required = false;
char *g_database_filename;
char *g_database_classified_dir;
bool first_classify;

typedef struct{
	string filename;
	string pattern;
} t_pattern;

typedef struct{
	int pattern;
	int file_index;
} l_class;


static void
define_messages()
{
}


static void
register_handlers()
{
}


void
shutdown_module(int signo)
{
	if (signo == SIGINT)
	{
		if (g_ipc_required)
			carmen_ipc_disconnect();
		exit(printf("rddf_graph_operations_on_graph_main: disconnected.\n"));
	}
}


void
create_image_3_3(cv::Mat *image_3_3, string pattern)
{
	int middle_x = ceil(image_3_3->rows/2);
	int middle_y = ceil(image_3_3->cols/2);
	int cont = 0;

	for (int x = middle_x - 1; x <= middle_x + 1; x++)
	{
		for (int y = middle_y - 1; y <= middle_y + 1; y++)
		{

			if(pattern[cont] == '1')
			{
				cv::Vec3b pixel_color (0, 0, 0);
				paint_image(x, y, pixel_color, image_3_3);
			}
			cont++;
		}
	}
}


int
convert_binary_to_decimal (int num)
{
	int decimal_val = 0, base = 1, rem;

	 while (num > 0)
	 {
		 rem = num % 10;
		 decimal_val = decimal_val + rem * base;
		 num = num / 10 ;
		 base = base * 2;
	 }

	 return (decimal_val);
}


void
classify_train_data(FILE *f_last_classified, vector < vector<t_pattern> > database_filenames, l_class last_classified, string database_classified_dir)
{
	cv::namedWindow("z->valid | x->invalid | esc->exit", cv::WINDOW_AUTOSIZE);
	cv::namedWindow("image 3x3", cv::WINDOW_AUTOSIZE);
	cv::moveWindow("image 3x3", 15*20, 0);
	cv::Mat image_15_15;
	cv::Mat image_15_15_scaled;
	cv::Mat image_3_3;
	cv::Mat image_3_3_scaled;
	cv::Size size(15 * 10, 15 * 10);
	int k;
	l_class actual;

	string filename = database_classified_dir + "/database_classified.txt";
	FILE *f_database_classified = fopen(filename.c_str(), "a+");

	vector <string> valids;
	vector <string> invalids;
	bool back_pressed = false;

	for (unsigned int i = last_classified.file_index; i<database_filenames.size();i++)
	{
		for (unsigned int j = last_classified.pattern; j<database_filenames[i].size();j++)
		//for (unsigned int j = last_classified.pattern; j<database_filenames[i].size();)
		{
			//if (database_filenames[i].size() != 0 )
			if (std::count(database_filenames[i][j].pattern.begin(), database_filenames[i][j].pattern.end(),'1') >= 3)
			//if(i==84)
			{
				actual.file_index = i;
				actual.pattern = j;
				image_15_15 = cv::Mat(15, 15, CV_8UC3, cv::Scalar(255, 255, 255));
				image_3_3 = cv::Mat(15, 15, CV_8UC3, cv::Scalar(255, 255, 255));
				image_15_15 = cv::imread(database_filenames[i][j].filename);
				if (!image_15_15.data)
				{
					std::cout << "Image " << database_filenames[i][j].filename <<  " not loaded"<<endl;
					exit(1);
				}
				create_image_3_3(&image_3_3, database_filenames[i][j].pattern);
				//cv::resize(image_15_15, image_15_15_scaled, size, 0, 0, cv::INTER_NEAREST);
				//cv::resize(image_15_15, image_15_15_scaled, size, 0, 0, cv::INTER_NEAREST);
				cv::resize(image_15_15, image_15_15_scaled, size);
				//cv::resize(image_3_3, image_3_3_scaled, size, 0, 0, cv::INTER_NEAREST);
				cv::resize(image_3_3, image_3_3_scaled, size);
				cv::imshow("z->valid | x->invalid | esc->exit", image_15_15_scaled);
				cv::imshow("image 3x3", image_3_3_scaled);

				k = (char)cv::waitKey();
				if (k == 122) //z - valido!!!
				{
					/*if(database_filenames[i][j].filename.compare(invalids.back()) == 0)
						invalids.pop_back();

					valids.push_back(database_filenames[i][j].filename);
					j++;*/


					string new_filename = database_filenames[i][j].filename;
					size_t found_bar = database_filenames[i][j].filename.find_last_of("/");
					new_filename = database_filenames[i][j].filename.substr(found_bar+1, database_filenames[i][j].filename.size()-found_bar-1);
					new_filename = database_classified_dir + "/" + new_filename;
					fprintf(f_database_classified, "%s %s\n", new_filename.c_str(), database_filenames[i][j].pattern.c_str());
					string command = "cp " + database_filenames[i][j].filename + " " + new_filename;
					system(command.c_str());
				}
				if (k == 120) //x - invalido!!!!!!
				{
					/*if(database_filenames[i][j].filename.compare(valids.back()) == 0)
						valids.pop_back();

					invalids.push_back(database_filenames[i][j].filename);
					j++;*/

					string new_filename = database_filenames[i][j].filename;
					size_t found_bar = database_filenames[i][j].filename.find_last_of("/");
					size_t found_underline = database_filenames[i][j].filename.find_last_of("_");
					new_filename = database_filenames[i][j].filename.substr(found_bar+1, found_underline-found_bar) + "0.jpg";
					new_filename = database_classified_dir + "/" + new_filename;
					fprintf(f_database_classified, "%s %s\n", new_filename.c_str(), database_filenames[i][j].pattern.c_str());
					string command = "cp " + database_filenames[i][j].filename + " " + new_filename;
					system(command.c_str());
				}
				if (k == 44) //traz - volta!!!!!!
				{
					j-=2;
					back_pressed = true;
				}

				if (k == 27) //sair
				{
					fprintf(f_last_classified, "%d %d\n", actual.file_index, actual.pattern);
					fclose (f_database_classified);
					fclose (f_last_classified);
					exit(1);
				}
			}
		}

	}

}


void
fill_database_matrix(FILE *f_database_filenames, vector < vector<t_pattern> > &database_filenames)
{
	int id, pattern_binary, pattern_decimal;
	//string str_filename;
	char filename[40];
	char pattern[9];
	int cont = 0;
	t_pattern p;


	while(fscanf(f_database_filenames, "%d %s %s\n", &id, filename, pattern) != EOF)
	{
		p.filename = filename;
		p.pattern = pattern;
		pattern_binary = atoi(p.pattern.c_str());
		pattern_decimal = convert_binary_to_decimal(pattern_binary);
		database_filenames[pattern_decimal].push_back(p);
		//cout<<id<<" "<<filename<<" "<<pattern_binary<<" "<<pattern_decimal<<endl;
		//getchar();
	}

	int sum = 0;
	for (unsigned int i = 0; i<database_filenames.size();i++)
	{
		if (database_filenames[i].size() != 0)
		{
			//if(i!=16)
				//sum+=database_filenames[i].size();
			cont++;
			//cout<<i<<" ";
			//cout<<database_filenames[i][0].filename<<endl;
			//cout<<database_filenames[i].size()<<endl;
		}
		//cout<<i<<" "<<database_filenames[i].size()<<endl;getchar();
	}
	cout<<"Database has "<<cont<<" different patterns "<<sum<<endl;
}


static void
read_parameters(int argc, char **argv)
{
	const char usage[] = "<database_dir>/<database_file>.txt <database_classified_dir> < -f(for first classification) or -c(continue classifying)>";
	if (argc < 3){
		printf("Incorrect Input!.\nUsage:\n%s %s\n", argv[0], usage);
		exit(1);
	}
	else
	{
		g_database_filename = argv[1];
		g_database_classified_dir = argv[2];
		if(strcmp(argv[3], "-f") == 0)
			first_classify = true;
		else
			first_classify = false;
	}
}


int
main(int argc, char **argv)
{
	read_parameters(argc,argv);

	if (g_ipc_required)
	{
		carmen_ipc_initialize(argc, argv);
		carmen_param_check_version(argv[0]);
		define_messages();
	}
	//signal(SIGINT, shutdown_module);

	printf("* * * * Database Classifier * * * \n");
	printf("* z: valid pattern               *\n");
	printf("* x: invalid pattern             *\n");
	printf("* esc: exit program              *\n");
	printf("* * * * * * * * * *  * * * * * * *\n");

	FILE *f_database_filenames;
	FILE *f_last_classified;
	vector < vector<t_pattern> > database_filenames(512);
	l_class last_classified;

	f_database_filenames = fopen (g_database_filename,"r");
	if(f_database_filenames == NULL)
	{
		printf("Graph file could not be read!\n");
		exit(1);
	}
	fill_database_matrix(f_database_filenames, database_filenames);
	fclose (f_database_filenames);

	string database_classified_dir(g_database_classified_dir);
	if(first_classify == true)
	{
		string filename = database_classified_dir + "/last_classified.txt";
		f_last_classified = fopen(filename.c_str(),"w");
		last_classified.file_index = 0;
		last_classified.pattern = 0;
	}
	else
	{
		string filename = database_classified_dir + "/last_classified.txt";
		f_last_classified = fopen (filename.c_str(),"r+");
		fscanf(f_last_classified, "%d %d\n", &last_classified.file_index, &last_classified.pattern);
	}


	classify_train_data (f_last_classified, database_filenames, last_classified, database_classified_dir);

	if (g_ipc_required)
	{
		register_handlers();
		carmen_ipc_dispatch();
	}

}
