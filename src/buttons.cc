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

#include <highgui.h>

#include "buttons.h"

using namespace cv;

void cvButtonsOnMouse( int e, int x, int y, int f, void* param ) {
	((CvButtons*)param)->setMouseState(e,x,y,f);
}

void CvButtons::paintButtons(Mat img) {
	std::vector<PushButton>::iterator it = buttonList.begin();

	while (it != buttonList.end()) {

		// Grab button variables:
		int x = (*it).x_pos;
		int y = (*it).y_pos;
		int w = (*it).width;
		int h = (*it).height;
		int x2 = x+w;
		int y2 = y+h;

		// Highlight mouseover position:
		if (mx >= x && mx <= x2 && my >= y && my <= y2) {

			rectangle(img, Point(x-2,y-2), Point(x2+2,y2+2),
					Scalar(255,255,255), 1, CV_AA);

			// Check for mouse pressed event:
			if (me == CV_EVENT_LBUTTONDOWN) {

				// Check if toggle button has to change state
				if ((*it).toggle == 0 || (*it).toggle == 1 )
					(*it).toggle = !(*it).toggle;

				// Call callback function
				(*it).cb((*it).toggle);

				// Draw confirmation rectangle:
				rectangle(img, Point(x,y), Point(x2,y2),
						Scalar(255,255,255), CV_FILLED, CV_AA);

				// Reset event (avoid flickering buttons):
				me = CV_EVENT_MOUSEMOVE;
			}
		}

		// Draw toggle state
		if ((*it).toggle == 1) {
			rectangle(img, Point(x,y), Point(x2,y2),
					Scalar(255,255,255), CV_FILLED, CV_AA);
		}

		// Draw button with text
		rectangle(img, Point(x,y), Point(x2,y2),
				Scalar(255,255,255), 1, CV_AA);
		if ((*it).toggle == 1) {
			putText(img, (*it).text,
					Point(x+10,y+15),
					FONT_HERSHEY_COMPLEX_SMALL, 0.5,
					Scalar(0,0,0), 1, CV_AA);
		} else {
			putText(img, (*it).text,
					Point(x+10,y+15),
					FONT_HERSHEY_COMPLEX_SMALL, 0.5,
					Scalar(255,255,255), 1, CV_AA);
		}

		// Step to next button
		++it;
	}
}
