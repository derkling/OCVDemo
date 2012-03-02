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

#include <algorithm>
#include <cstdio>
#include <bbque/utils/timer.h>
#include <bbque/utils/utility.h>

#include "ocvdemo_exc.h"

// These are a set of useful debugging log formatters
#define FMT_DBG(fmt) BBQUE_FMT(COLOR_LGRAY,  "TESTA_EXC  [DBG]", fmt)
#define FMT_INF(fmt) BBQUE_FMT(COLOR_GREEN,  "TESTA_EXC  [INF]", fmt)
#define FMT_WRN(fmt) BBQUE_FMT(COLOR_YELLOW, "TESTA_EXC  [WRN]", fmt)
#define FMT_ERR(fmt) BBQUE_FMT(COLOR_RED,    "TESTA_EXC  [ERR]", fmt)

using namespace bbque::utils;
using namespace cv;

OCVDemo::Resolution OCVDemo::resolutions[] = {
	{ 320,  240},
	{ 640,  480},
	{1280, 1024}
};

OCVDemo::OCVDemo(std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t *rtlib,
		uint8_t cid, uint8_t fps_max) :
	BbqueEXC(name, recipe, rtlib) {


	// Keep track of the WebCam ID managed by this instance
	cam.id = cid;
	cam.fps_max = fps_max;
	cam.frames_count = 0;
	cam.frames_total = 0;
	fprintf(stderr, FMT_WRN("OpenCV Demo EXC (webcam %d, max %d [fps]\n"),
				cam.id, fps_max);

	// Setup default constraint
	cnstr.awm = AWM_START_ID;
	cnstr.operation = CONSTRAINT_ADD;
	cnstr.type = UPPER_BOUND;
	SetConstraints(&cnstr, 1);
	fprintf(stderr, FMT_INF("AWM ID init upper bound: %d\n"), cnstr.awm);

}

OCVDemo::~OCVDemo() {

}

RTLIB_ExitCode_t OCVDemo::SetResolution(uint8_t type) {
	if (type >= RES_COUNT)
		return RTLIB_ERROR;

	cam.resolution_idx = type;
	cam.cap.set(CV_CAP_PROP_FRAME_WIDTH,  CAM_WIDTH(cam));
	cam.cap.set(CV_CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT(cam));
	fprintf(stderr, "Set camera resolution: %d x %d\n",
			CAM_WIDTH(cam), CAM_HEIGHT(cam));
	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::onSetup() {
	char wcap[] = "CAM99";

	// Setup camera name
	snprintf(wcap, sizeof(wcap), "CAM%02d", cam.id);
	cam.wcap = std::string(wcap);

	// Open input device
	fprintf(stderr, "Opening V4L2 device [/dev/video%d]...\n", cam.id);
	cam.cap = VideoCapture(cam.id);
	if (!cam.cap.isOpened()) {
		fprintf(stderr, "ERROR: %s opening FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	// Set initial camara resolution to medium
	SetResolution(RES_MID);

	// Setup camera view
	namedWindow(cam.wcap.c_str(), CV_WINDOW_AUTOSIZE);

	return RTLIB_OK;
}



RTLIB_ExitCode_t OCVDemo::onConfigure(uint8_t awm_id) {
	fprintf(stderr, FMT_WRN("OCVDemo::onConfigure(): "
				"EXC [%s], AWM[%02d]\n"),
				exc_name.c_str(), awm_id);

	// Get the start processing time
	tstart = bbque_tmr.getElapsedTimeMs();
	cam.frames_count = 0;
	return RTLIB_OK;
}


RTLIB_ExitCode_t OCVDemo::getImage() {

	// Start next frame grabbing for each camera
	if (!cam.cap.grab()) {
		fprintf(stderr, "ERROR: %s frame grabbing FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	// Acquire the frame from camera
	if (!cam.cap.retrieve(cam.frame)) {
		fprintf(stderr, "ERROR: %s frame retriving FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::showImage() {
	imshow(cam.wcap.c_str(), cam.frame);
	return RTLIB_OK;
}

double OCVDemo::updateFps() {
	static double elapsed_ms = 0; // [ms] elapsed since start
	static double update_ms = 1000; // [s] to next console update
	double tnow; // [s] at the call time

	tnow = bbque_tmr.getElapsedTimeMs();
	++cam.frames_count;
	++cam.frames_total;

	elapsed_ms = tnow - tstart;
	if (elapsed_ms > update_ms) {
		cam.fps_curr = cam.frames_count * 1000 / elapsed_ms;
		DB(fprintf(stderr, "Processing @ FPS = %.2f\n", cam.fps_curr));
		update_ms += 1000;
	}

	return cam.fps_curr;
}

RTLIB_ExitCode_t OCVDemo::onRun() {

	// Acquired a new images
	getImage();

	// Update FPS accounting
	updateFps();

	// Show the current image
	showImage();

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::onMonitor() {
	uint8_t key = (cvWaitKey(1) & 255);

	switch (key) {
	case 27:
		return RTLIB_EXC_WORKLOAD_NONE;
	case '+':
		IncUpperAwmID();
		break;
	case '-':
		DecUpperAwmID();
		break;
	}

	return RTLIB_OK;
}

void OCVDemo::IncUpperAwmID() {

	if (cnstr.awm == AWM_UPPER_ID)
		return;

	++cnstr.awm;
	SetConstraints(&cnstr, 1);

	fprintf(stderr, FMT_INF("Enabled AWM: [0, %d]\n"), cnstr.awm);
}

void OCVDemo::DecUpperAwmID() {

	if (cnstr.awm == 0)
		return;

	--cnstr.awm;
	SetConstraints(&cnstr, 1);

	fprintf(stderr, FMT_INF("Enabled AWM: [0, %d]\n"), cnstr.awm);
}

