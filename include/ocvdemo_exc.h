/* Copyright (C) 2012  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_OPENCV_DEMO_EXC_H_
#define BBQUE_OPENCV_DEMO_EXC_H_

#include <opencv2/opencv.hpp>
#include <bbque/bbque_exc.h>

#define AWM_START_ID 	1
#define AWM_UPPER_ID 	2

using bbque::rtlib::BbqueEXC;
using cv::VideoCapture;
using cv::Mat;

class OCVDemo : public BbqueEXC {

public:

	OCVDemo(std::string const & name,
			std::string const & recipe,
			RTLIB_Services_t *rtlib,
			uint8_t cid, uint8_t fps_max);

	virtual ~OCVDemo();

private:

	struct Camera {
		uint8_t id;
		uint8_t fps_max;
		VideoCapture cap;
		Mat frame;
		std::string wcap;
	} cam;

	RTLIB_Constraint_t cnstr;

	void IncUpperAwmID();
	void DecUpperAwmID();
	RTLIB_ExitCode_t getImage();
	RTLIB_ExitCode_t showImage();

	RTLIB_ExitCode_t onSetup();
	RTLIB_ExitCode_t onConfigure(uint8_t awm_id);
	RTLIB_ExitCode_t onRun();
	RTLIB_ExitCode_t onMonitor();
};

#endif // BBQUE_OPENCV_DEMO_EXC_H_

