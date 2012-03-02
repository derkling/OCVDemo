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
#include "buttons.h"

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

const char *OCVDemo::effectStr[] = {
	"None",
	"Canny"
};

/*******************************************************************************
 * Golbal GUI Elements
 ******************************************************************************/

CvButtons *buttons;

bool evtExit = false;
void on_exit(int toggle) {
	evtExit = true;
}

/*******************************************************************************
 * OpenCV Demo Code
 ******************************************************************************/

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
	cam.effect_idx = EFF_NONE;
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

	// Create simple buttons and attach them to their callback functions
	buttons = new CvButtons();
	buttons->addButton(PushButton(10, 10, 110, 20, -1, "Exit", on_exit));
	cvSetMouseCallback(cam.wcap.c_str(), cvButtonsOnMouse, buttons);

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

#define TEST_FONT(YPOS, TYPE)\
	putText(cam.frame,\
			"Test font: " #TYPE,\
			Point(15, 15*YPOS),\
			TYPE, 0.5,\
			Scalar(0,0,0), 1, CV_AA);

RTLIB_ExitCode_t OCVDemo::showImage() {
	uint16_t xorg = CAM_WIDTH(cam)  - 240;
	uint16_t yorg = CAM_HEIGHT(cam) -  30;
	uint16_t xend = CAM_WIDTH(cam)  -   5;
	uint16_t yend = CAM_HEIGHT(cam) -   5;
	uint16_t xthm = CAM_WIDTH(cam)  -   5;
	uint8_t  next_line = 1; // The first test line to write
	char buff[64]; // auxiliary text buffer
	// The image to be displayed (by default the captured frame)
	Mat display = cam.frame;
	Mat roi; // A generic image ROI
#define LINE_YSPACE 11
#define TEXT_LINE(IMG, TXT)\
	if ( 1 ) {\
	putText(IMG, TXT,\
		Point(xorg + 5, yorg + (LINE_YSPACE * next_line)),\
		FONT_HERSHEY_COMPLEX_SMALL, 0.5,\
		Scalar(200,200,200), 1, CV_AA);\
	++next_line;\
	}

#if 0
	TEST_FONT(0, FONT_HERSHEY_SIMPLEX);
	TEST_FONT(1, FONT_HERSHEY_PLAIN);
	TEST_FONT(2, FONT_HERSHEY_DUPLEX);
	TEST_FONT(3, FONT_HERSHEY_COMPLEX);
	TEST_FONT(4, FONT_HERSHEY_TRIPLEX);
	TEST_FONT(5, FONT_HERSHEY_COMPLEX_SMALL);
	TEST_FONT(6, FONT_HERSHEY_SCRIPT_SIMPLEX);

	// Overlay the information box
	rectangle(cam.frame,
			Point(xorg, yorg),
			Point(xend, yend),
			Scalar(63,103,157),
			CV_FILLED);
#endif

	// Render frame as thumbnail if effects are enabled
	if (cam.effect_idx != EFF_NONE) {
		display = cam.effects;
		xthm -= round(cam.frame.cols*0.25);
		roi = display(Rect(xthm, 10,
			round(cam.frame.cols*0.25),
			round(cam.frame.rows*0.25)
		));
		resize(cam.frame, roi, roi.size());
	}

	snprintf(buff, 64,
		"/dev/video%d: "
		"%dx%d @ %5.2f [fps]",
		cam.id,
		CAM_WIDTH(cam), CAM_HEIGHT(cam), cam.fps_curr
	);
	TEXT_LINE(display, buff);

	snprintf(buff, 64,
		"AWMs: %d,%d [cur,max] | "
		"%s",
		CurrentAWM(), cnstr.awm, effectStr[cam.effect_idx]
	);
	TEXT_LINE(display, buff);

	// Update buttons
	buttons->paintButtons(display);

	imshow(cam.wcap.c_str(), display);

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::doCanny() {
	cvtColor(cam.frame, cam.effects, CV_BGR2GRAY);
	GaussianBlur(cam.effects, cam.effects, Size(7,7), 1.5, 1.5);
	Canny(cam.effects, cam.effects, 0, 30, 3);
	// Ensure image is 3 channel RGB, as required for proper
	// composition with overlay info
	cvtColor(cam.effects, cam.effects, CV_GRAY2RGB);
	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::postProcess() {

	if (cam.effect_idx == EFF_NONE)
		return RTLIB_OK;

	switch (cam.effect_idx) {
	case EFF_CANNY:
		doCanny();
		break;
	default:
		fprintf(stderr, "Unknowen effect required\n");
		return RTLIB_ERROR;
	}

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

	// Apply required effects
	postProcess();

	// Update FPS accounting
	updateFps();

	// Show the current image
	showImage();

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::onMonitor() {
	uint8_t key = (cvWaitKey(1) & 255);

	// Exit when the used press the "Exit" Button
	if (evtExit)
		return RTLIB_EXC_WORKLOAD_NONE;

	switch (key) {
	case 27:
		return RTLIB_EXC_WORKLOAD_NONE;
	case '+':
		IncUpperAwmID();
		break;
	case '-':
		DecUpperAwmID();
		break;
	case 'c':
		fprintf(stderr, "Enable [CANNY] effect\n");
		cam.effect_idx = EFF_CANNY;
		break;
	case 'q':
		fprintf(stderr, "Disable effects\n");
		cam.effect_idx = EFF_NONE;
		break;
	}

	return RTLIB_OK;
}

void OCVDemo::IncUpperAwmID() {

	if (cnstr.awm == AWM_UPPER_ID)
		return;

	++cnstr.awm;
	SetConstraints(&cnstr, 1);

	if (cnstr.awm <= RES_COUNT)
		SetResolution(cnstr.awm);

	fprintf(stderr, FMT_INF("Enabled AWM: [0, %d]\n"), cnstr.awm);
}

void OCVDemo::DecUpperAwmID() {

	if (cnstr.awm == 0)
		return;

	--cnstr.awm;
	SetConstraints(&cnstr, 1);

	if (cnstr.awm <= RES_COUNT)
		SetResolution(cnstr.awm);

	fprintf(stderr, FMT_INF("Enabled AWM: [0, %d]\n"), cnstr.awm);
}

