

#ifndef __SEGMAP_COLORMAPS_H__
#define __SEGMAP_COLORMAPS_H__

#include <vector>
#include <opencv/cv.hpp>


class CityScapesColorMap
{
	std::vector<cv::Scalar> _colors;

public:
	int n_classes;

	CityScapesColorMap()
	{
		n_classes = 21;

		_colors.push_back(cv::Scalar(128, 64, 128));
		_colors.push_back(cv::Scalar(244, 35, 232));
		_colors.push_back(cv::Scalar(70, 70, 70));
		_colors.push_back(cv::Scalar(102, 102, 156));
		_colors.push_back(cv::Scalar(190, 153, 153));
		_colors.push_back(cv::Scalar(153, 153, 153));
		_colors.push_back(cv::Scalar(250, 170, 30));
		_colors.push_back(cv::Scalar(220, 220, 0));
		_colors.push_back(cv::Scalar(107, 142, 35));
		_colors.push_back(cv::Scalar(152, 251, 152));
		_colors.push_back(cv::Scalar(70, 130, 180));
		_colors.push_back(cv::Scalar(220, 20, 60));
		_colors.push_back(cv::Scalar(255, 0, 0));
		_colors.push_back(cv::Scalar(0, 0, 142));
		_colors.push_back(cv::Scalar(0, 0, 70));
		_colors.push_back(cv::Scalar(0, 60, 100));
		_colors.push_back(cv::Scalar(0, 80, 100));
		_colors.push_back(cv::Scalar(0, 0, 230));
		_colors.push_back(cv::Scalar(119, 11, 32));
		_colors.push_back(cv::Scalar(0, 0, 0)); // unknown class
		_colors.push_back(cv::Scalar(255, 255, 255)); // lane marks
	}

	cv::Scalar color(int index)
	{
		return _colors[index % _colors.size()];
	}
};


#endif
