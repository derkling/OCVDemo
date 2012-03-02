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

	double tstart;

	enum ResolutionType {
		RES_LOW = 0,
		RES_MID,
		RES_HIG,
		RES_COUNT // This must be the last element
	};

	enum EffectType {
		EFF_NONE = 0,
		EFF_CANNY,
		EFF_COUNT // This must be the last element
	};

	static const char *effectStr[EFF_COUNT];

	static struct Resolution {
		uint16_t width;
		uint16_t height;
	} resolutions[RES_COUNT];

	struct Camera {
		uint8_t id;
		uint8_t fps_max;
		float fps_curr;
		uint32_t frames_count;
		uint32_t frames_total;

		VideoCapture cap;
		Mat frame;
		std::string wcap;

		// Current camera resolution
		uint8_t resolution_idx;

		// The effect to apply at the image
		uint8_t effect_idx;
		Mat effects;
	} cam;
#define CAM_WIDTH(CAM) \
	resolutions[CAM.resolution_idx].width
#define CAM_HEIGHT(CAM) \
	resolutions[CAM.resolution_idx].height


	RTLIB_Constraint_t cnstr;

	RTLIB_ExitCode_t SetResolution(uint8_t type);
	void IncUpperAwmID();
	void DecUpperAwmID();
	RTLIB_ExitCode_t getImage();
	RTLIB_ExitCode_t showImage();
	double updateFps();

	RTLIB_ExitCode_t doCanny();
	RTLIB_ExitCode_t postProcess();

	RTLIB_ExitCode_t onSetup();
	RTLIB_ExitCode_t onConfigure(uint8_t awm_id);
	RTLIB_ExitCode_t onRun();
	RTLIB_ExitCode_t onMonitor();
};

#endif // BBQUE_OPENCV_DEMO_EXC_H_
