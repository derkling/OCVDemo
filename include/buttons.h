/* Copyright (C) 2012  Politecnico di Milano
 *
 * Base on original code from: Andreas Geiger <geiger@kit.edu>
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

#ifndef BBQUE_OPENCV_DEMO_BUTTONS_H_
#define BBQUE_OPENCV_DEMO_BUTTONS_H_

#include <cv.h>
#include <vector>

using cv::Mat;

/**
 * Global OpenCV mouse button callback.
 * Must be set with cvSetMouseCallback(..) to make buttons work.
 */
void cvButtonsOnMouse (int event, int x, int y, int flags, void* param);


/**
 * @brief Implements a single push button object
 */

class PushButton {

public:

	/**
	 * @brief Build a new PushButton
	 *
	 * Constructor takes parameters such as:
	 * - \b x , \b y : x/y position of a push button
	 * - \b w , \b h : width/height of a push button
	 * - \b t: -1 if normal button or 0/1 as state of a toggle button
	 * - \b text: button description
	 * - \b cb: button callback function. Takes a function pointer.
	 * The argument will be the button toggle state when pressed.
	 */
	PushButton (int x, int y, int w, int h, int t,
			const char *text, void (*cb)(int)) :
		x_pos(x), y_pos(y), width(w), height(h),
		toggle(t), text(text), cb(cb) {
	}

	int x_pos, y_pos;
	int width, height;
	int toggle;
	const char *text;
	void (*cb)(int);
};


/**
 * @brief A push button manager
 *
 * Implements functions to enhance the OpenCV GUI elements
 * by simple, platform-independet push buttons and toggle elements.\n
 */
class CvButtons {

public:

	CvButtons() {
	}

	~CvButtons() {
		buttonList.clear();
	}

	void setMouseState(int e, int x, int y, int f) {
		me=e;
		mx=x;
		my=y;
		mf=f;
	}

	void paintButtons(Mat img);

	void addButton(PushButton pb) {
		buttonList.push_back(pb);
	}

	void delButton(int pos) {
		buttonList.erase(buttonList.begin()+pos);
	}

private:

	std::vector<PushButton> buttonList;
	int me, mx, my, mf;
};

#endif // BBQUE_OPENCV_DEMO_BUTTONS_H_
