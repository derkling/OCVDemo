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
#include <ctime>
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

const char *OCVDemo::resolutionStr[] = {
	"LOW",
	"MID",
	"HIG"
};

const char *OCVDemo::effectStr[] = {
	"None",
	"Canny",
	"FAST",
	"SURF"
};

/*******************************************************************************
 * Golbal GUI Elements
 ******************************************************************************/

CvButtons *buttons;

bool evtExit = false;
void on_exit(int toggle) {
	evtExit = true;
}

bool evtSnapshot;
void on_snapshot(int toggle) {
	evtSnapshot = true;
}

/*******************************************************************************
 * OpenCV Demo Code
 ******************************************************************************/

OCVDemo::OCVDemo(std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t *rtlib,
		std::string const & video,
		uint8_t cid, uint8_t fps_max,
		uint32_t frames_max) :
	BbqueEXC(name, recipe, rtlib) {


	// Keep track of the WebCam ID managed by this instance
	cam.id = cid;
	cam.video = video;
	cam.using_camera = true;
	if (cam.video != "")
		cam.using_camera = false;
	cam.fps_max = fps_max;
	cam.frames_count = 0;
	cam.frames_total = 0;
	cam.frames_max = frames_max;
	cam.effect_idx = EFF_NONE;
	if (CAMERA_SOURCE) {
		fprintf(stderr, FMT_WRN("OpenCV Demo EXC (webcam %d, max %d [fps]\n"),
				cam.id, cam.fps_max);
	} else {
		fprintf(stderr, FMT_WRN("OpenCV Demo EXC (video %s, max %d [fps]\n"),
				cam.video.c_str(), cam.fps_max);
	}
	if (cam.frames_max) {
		fprintf(stderr, FMT_WRN("Decoding up-to %d frames\n"), cam.frames_max);
	}

	// Setup default constraint
	cnstr.awm = AWM_START_ID;
	cnstr.operation = CONSTRAINT_ADD;
	cnstr.type = UPPER_BOUND;
	SetConstraints(&cnstr, sizeof(cnstr)/sizeof(RTLIB_Constraint_t));
	fprintf(stderr, FMT_INF("AWM ID init upper bound: %d\n"), cnstr.awm);

}

OCVDemo::~OCVDemo() {

}


RTLIB_ExitCode_t OCVDemo::SetResolutionCamera(uint8_t type) {

	fprintf(stderr, "Setup camera resolution: [%d x %d]...\n",
			CAM_PRESET_WIDTH(type), CAM_PRESET_HEIGHT(type));

	cam.cap.set(CV_CAP_PROP_FRAME_WIDTH, CAM_PRESET_WIDTH(type));
	cam.cap.set(CV_CAP_PROP_FRAME_HEIGHT, CAM_PRESET_HEIGHT(type));

	// Keep track of current camera resolution
	cam.cur_res.width = CAM_PRESET_WIDTH(type);
	cam.cur_res.height = CAM_PRESET_HEIGHT(type);

	cam.reduce_fct = 1.00;

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::SetResolutionVideo(uint8_t type) {

	// Video source resolution is changed at frame acquisition time
	// Once this method is called, just setup the required actual sizes
	switch (type) {
	case RES_LOW:
		cam.reduce_fct = 0.33;
		break;
	case RES_MID:
		cam.reduce_fct = 0.66;
		break;
	default:
		cam.reduce_fct = 1.00;
	}

	// Keep track of current camera resolution
	cam.cur_res.width = round(cam.max_res.width * cam.reduce_fct);
	cam.cur_res.height = round(cam.max_res.height * cam.reduce_fct);

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::SetResolution(uint8_t type) {
	RTLIB_ExitCode_t result = RTLIB_OK;

	if (type >= RES_COUNT)
		return RTLIB_ERROR;

	if (CAMERA_SOURCE) {
		result = SetResolutionCamera(type);
	} else {
		result = SetResolutionVideo(type);
	}
	if (result != RTLIB_OK) {
		return result;
	}

	// Keep track of current camera resolution
	cam.res_id = type;
	fprintf(stderr, "Current resolution %s: [%d x %d]...\n",
			resolutionStr[cam.res_id],
			CAM_WIDTH(cam), CAM_HEIGHT(cam));

	return result;

}

RTLIB_ExitCode_t OCVDemo::SetupSourceVideo() {
	Mat frame;

	fprintf(stderr, "Opening video [%s]...\n", cam.video.c_str());
	cam.cap = VideoCapture(cam.video);

	// Check if video soure has been properly initialized
	if (!cam.cap.isOpened()) {
		fprintf(stderr, "ERROR: opening video [%s] FAILED!\n",
				cam.video.c_str());
		return RTLIB_ERROR;
	}

	// Get first frame which is used to read the native resolution
	cam.cap >> frame;
	if (frame.empty()) {
		fprintf(stderr, "ERROR: %s frame grabbing FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	// Setup maximum (native) resoulution
	cam.max_res.width = frame.cols;
	cam.max_res.height = frame.rows;
	if (!cam.max_res.width || !cam.max_res.height) {
		return RTLIB_ERROR;
	}

	cam.using_camera = false;
	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::SetupSourceCamera() {
	char wcap[] = "CAM99";

	// Setup camera name
	snprintf(wcap, sizeof(wcap), "CAM%02d", cam.id);
	cam.wcap = std::string(wcap);

	// Open input device
	fprintf(stderr, "Opening V4L2 device [/dev/video%d]...\n", cam.id);
	cam.cap = VideoCapture(cam.id);

	// Check if video soure has been properly initialized
	if (!cam.cap.isOpened()) {
		fprintf(stderr, "ERROR: opening camera [%s] FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	// Setup maximum (native) resoulution
	cam.max_res.width = CAM_PRESET_WIDTH(RES_HIG);
	cam.max_res.height = CAM_PRESET_HEIGHT(RES_HIG);

	cam.using_camera = true;
	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::onSetup() {
	RTLIB_ExitCode_t result;

	// Setup the required video source
	if (CAMERA_SOURCE) {
		result = SetupSourceCamera();
	} else {
		result = SetupSourceVideo();
	}
	if (result != RTLIB_OK)
		return result;

	fprintf(stderr, "Max (native) resolution: [%d x %d]\n",
			cam.max_res.width, cam.max_res.height);

	// Setup initial resolution to medium
	SetResolution(RES_MID);

	// Setup camera view
	namedWindow(cam.wcap.c_str(), CV_WINDOW_AUTOSIZE);

	// Create simple buttons and attach them to their callback functions
	buttons = new CvButtons();
	buttons->addButton(PushButton(10, 10, 110, 20, -1, "Exit", on_exit));
	buttons->addButton(PushButton(10, 40, 110, 20, -1, "Snapshot", on_snapshot));
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

	// Start next frame grabbing
	if (!cam.cap.grab()) {
		fprintf(stderr, "ERROR: %s frame grabbing FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::getImageFromVideo() {
	std::vector<DataMatrixCode> codes;
	Mat frame;

	// Scaled down the frame (if required)
	if (cam.reduce_fct < 1.0) {
		if (!cam.cap.read(frame))
			goto exit_eof;
		resize(frame, cam.frame,
				Size(round(frame.cols * cam.reduce_fct),
					round(frame.rows * cam.reduce_fct)));
	} else if (!cam.cap.read(cam.frame)) {
			goto exit_eof;
	}
	if (cam.frame.empty()) {
		fprintf(stderr, "ERROR: video frame grabbing FAILED!\n");
		return RTLIB_ERROR;
	}

	return RTLIB_OK;

exit_eof:
	return RTLIB_EXC_WORKLOAD_NONE;

}

RTLIB_ExitCode_t OCVDemo::getImageFromCamera() {

	// Acquire a frame from the camera
	if (!cam.cap.retrieve(cam.frame)) {
		fprintf(stderr, "ERROR: %s frame retriving FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	// Start next frame grabbing
	if (!cam.cap.grab()) {
		fprintf(stderr, "ERROR: %s frame grabbing FAILED!\n",
				cam.wcap.c_str());
		return RTLIB_ERROR;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::getImage() {
	if (CAMERA_SOURCE)
		return getImageFromCamera();
	return getImageFromVideo();
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

	// The image to be displayed (by default the captured frame)
	display = cam.frame;

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
		CAM_WIDTH(cam), CAM_HEIGHT(cam), cam.fps_cur
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

RTLIB_ExitCode_t OCVDemo::doFast() {
	// FAST Detector with (threshold = 10 and nonmax_suppression)
	FastFeatureDetector fastd(10, true);
	FeatureDetector* fd = &fastd;
	std::vector<KeyPoint> keypoints;

	// Get a gray image from the current frame
	cvtColor(cam.frame, cam.effects, CV_BGR2GRAY);

	// Keypoints detaction
	fd->detect(cam.effects, keypoints);

	// Ensure image is 3 channel RGB, as required for proper
	// composition with overlay info.
	// This allows also to draw colored cycrles for each keypoint
	cvtColor(cam.effects, cam.effects, CV_GRAY2RGB);

	//draw green circles where the keypoints are located
	vector<KeyPoint>::const_iterator it = keypoints.begin();
	for ( ; it != keypoints.end(); ++it) {
		circle(cam.effects, it->pt, 4, Scalar(0,0,255,0));
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::doSurf() {
	// SURF Detector with (hessianThreshold = 400., octaves = 3, octaveLayers = 4)
	SurfFeatureDetector surfd(400.0, 3, 4);
	FeatureDetector* fd = &surfd;
	std::vector<KeyPoint> keypoints;

	// Get a gray image from the current frame
	cvtColor(cam.frame, cam.effects, CV_BGR2GRAY);

	// Keypoints detaction
	fd->detect(cam.effects, keypoints);

	// Ensure image is 3 channel RGB, as required for proper
	// composition with overlay info.
	// This allows also to draw colored cycrles for each keypoint
	cvtColor(cam.effects, cam.effects, CV_GRAY2RGB);

	//draw green circles where the keypoints are located
	vector<KeyPoint>::const_iterator it = keypoints.begin();
	for ( ; it != keypoints.end(); ++it) {
		circle(cam.effects, it->pt, 4, Scalar(0,0,255,0));
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::postProcess() {

	if (cam.effect_idx == EFF_NONE)
		return RTLIB_OK;

	switch (cam.effect_idx) {
	case EFF_CANNY:
		doCanny();
		break;
	case EFF_FAST:
		doFast();
		break;
	case EFF_SURF:
		doSurf();
		break;
	default:
		fprintf(stderr, "Unknowen effect required\n");
		return RTLIB_ERROR;
	}

	return RTLIB_OK;
}

double OCVDemo::updateFps() {
	static double elapsed_ms = 0; // [ms] elapsed since start
	static double update_ms = tstart + 250.0; // [ms] to next console update
	double tnow; // [s] at the call time

	tnow = bbque_tmr.getElapsedTimeMs();
	++cam.frames_count;
	++cam.frames_total;

	if (tnow >= update_ms) {
		elapsed_ms = tnow - tstart;
		cam.fps_cur = cam.frames_count * 1000.0 / elapsed_ms;
		DB(fprintf(stderr, FMT_DBG("Processing @ FPS = %.2f\n"), cam.fps_cur));
		// Setup references for next update
		tstart = bbque_tmr.getElapsedTimeMs();
		update_ms = tstart + 250.0;
		cam.frames_count = 0;
	}

	return cam.fps_cur;
}

void OCVDemo::forceFps() {
	static double tstart = 0; // [ms] at the cycle start time
	float delay_ms = 0; // [ms] delay to stick with the required FPS
	uint32_t sleep_us;
	float cycle_time;
	float expec_time;
	double tnow; // [s] at the call time

	if (unlikely(tstart == 0)) {
		// The first frame is used to setup the start time
		tstart = bbque_tmr.getElapsedTimeMs();
		return;
	}

	tnow = bbque_tmr.getElapsedTimeMs();
//	fprintf(stderr, FMT_INF("TP: %.4f, TN: %.4f\n"),
//			tstart, tnow);

	cycle_time = tnow - tstart;
	expec_time = static_cast<float>(1e3) / cam.fps_max;
	delay_ms = expec_time - cycle_time;
	sleep_us = 1e3 * static_cast<uint32_t>(delay_ms);

	// Keep track of the framerate deviation
	cam.fps_dev = expec_time / cycle_time;

	if (cycle_time < expec_time) {
		DB(fprintf(stderr, FMT_INF("Cycle Time: %3.3f[ms], ET: %3.3f[ms], "
						"Sleep time %u [us]\n"),
			cycle_time, expec_time, sleep_us));
		usleep(sleep_us);
	}

	// Update the start time of the next cycle
	tstart = bbque_tmr.getElapsedTimeMs();
}

RTLIB_ExitCode_t OCVDemo::onRun() {
	RTLIB_ExitCode_t result;

	// Acquired a new images
	result = getImage();
	if (result != RTLIB_OK)
		return result;

	// Apply required effects
	postProcess();

	// Update FPS accounting
	updateFps();

	// Show the current image
	showImage();

	// Pad cycle time to force the maximum required framerate
	forceFps();

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::FrameratePolicy() {
	static bool napped = false;
	static uint16_t tcheck = 1000;
	static uint8_t urc = 0;
	static double tprv = 0;
	bool scaling = false;
	double tdif;
	double tnow;
	uint8_t nap;

	// Avoid adjustments more frequent than once every 5[s]
	tnow = bbque_tmr.getElapsedTimeMs();
	tdif = tnow - tprv;
	if (tdif < tcheck)
		return RTLIB_OK;

	// New adjustment time
	tprv = tnow;

	// Check if the current FPS is at least 80% of the required FPS
	if (cam.fps_dev >= 0.85) {
		napped = false;
		tcheck = 1000;
		urc = 4;
		return RTLIB_OK;
	}

	// Critical framerate deviation: increasing check frequency
	fprintf(stderr, FMT_INF("framerate deviation: %5.1f[%%]\r"),
			(1 - cam.fps_dev) * 100);
	tcheck = 1000;


	// Here we are at leat 20% under the required framerate

	// Effects disabled: scale-down on under FPS
	if (cam.effect_idx == EFF_NONE) {
		scaling = ResolutionDown();
		if (scaling)
			fprintf(stderr, FMT_INF("Under framerate %.1f[%%]\n"),
					cam.fps_dev * 100);
		return RTLIB_OK;
	}

	// Effect enabled: ask for more resources (if not already done)
	if (napped) {
		// NAP request timedout: reducing resolution
		scaling = ResolutionDown();
		if (scaling)
			fprintf(stderr, FMT_INF("NAP timeout: downscaling\n"));
		napped = false;
		return RTLIB_OK;
	}

	// Avoid NAPs if we are already at the maximum AWM
	if (CurrentAWM() == 2)
		return RTLIB_OK;

	// NAP not asserted: asserting a new one
	nap = (static_cast<uint8_t>((1 - cam.fps_dev) * 100) % 100);
	fprintf(stderr, FMT_INF("NAP assert [%d]\n"), nap);
	SetGoalGap(nap);
	napped = true;

	return RTLIB_OK;
}

RTLIB_ExitCode_t OCVDemo::onMonitor() {
	uint8_t key = (cvWaitKey(1) & 255);

	// Exit if we decoded the required amount of frames
	if (cam.frames_max &&
		cam.frames_total == cam.frames_max)
		return RTLIB_EXC_WORKLOAD_NONE;

	// Exit when the used press the "Exit" Button
	if (evtExit)
		return RTLIB_EXC_WORKLOAD_NONE;

	if (evtSnapshot)
		Snapshot();

	switch (key) {
	case 27:
		return RTLIB_EXC_WORKLOAD_NONE;
	case '+':
		ResolutionUp();
		break;
	case '-':
		ResolutionDown();
		break;
	case 'c':
		fprintf(stderr, "Enable [CANNY] effect\n");
		cam.effect_idx = EFF_CANNY;
		break;
	case 'f':
		fprintf(stderr, "Enable [FAST] effect\n");
		cam.effect_idx = EFF_FAST;
		break;
	case 's':
		fprintf(stderr, "Enable [SURF] effect\n");
		cam.effect_idx = EFF_SURF;
		break;
	case 'q':
		fprintf(stderr, "Disable effects\n");
		cam.effect_idx = EFF_NONE;
		break;
	}

	FrameratePolicy();
	return RTLIB_OK;
}

bool OCVDemo::ResolutionUp() {

	if (cam.res_id == RES_HIG)
		return false;

	fprintf(stderr, FMT_INF("Resolution Scale UP\n"));
	++cam.res_id;
	SetResolution(cam.res_id);
	return true;
}

bool OCVDemo::ResolutionDown() {

	if (cam.res_id == RES_LOW)
		return false;

	fprintf(stderr, FMT_INF("Resolution Scale DOWN\n"));
	--cam.res_id;
	SetResolution(cam.res_id);
	return true;
}

void OCVDemo::Snapshot() const {
	time_t ltime = time(NULL);
	struct tm *tm = localtime(&ltime);
	char timestamp[] = "20120302_193300";
	char filename[] = "/tmp/ocvdemo_display_20120302_193400.png";

	sprintf(timestamp,  "%04d%02d%02d_%02d%02d%02d",
			tm->tm_year+1900, tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	sprintf(filename, "/tmp/ocvdemo_frame_%s.png", timestamp);
	imwrite(filename, cam.frame);
	sprintf(filename, "/tmp/ocvdemo_display_%s.png", timestamp);
	imwrite(filename, display);
	evtSnapshot = false;
}
