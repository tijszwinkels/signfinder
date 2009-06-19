/*
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is maskMaker.
 *
 * The Initial Developer of the Original Code is Tijs Zwinkels.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Tijs Zwinkels <opensource AT tumblecow DOT net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */


#include "ImageHandler.h"
#include <opencv/highgui.h>
#include <iostream>
#include <math.h>

/*
 * Support-class for 'maskMaker'.
 *
 * Encapculates most of the logic, such as: Loading files, managing image buffers, 
 * and calculating and managing mouse-click for the tools.
 *
 * June 2009, Tijs Zwinkels <opensource AT tumblecow DOT net>
 */


using namespace std;

const static int BUFSIZE = 256;

/* Constructor / Destructor -------------------------------------------------- */
ImageHandler::ImageHandler()
{
	init();
}
	
ImageHandler::~ImageHandler()
{
	delete _polygonClick;
}


/* Public Functions --------------------------------------------------------- */

/* Load an image-file */
void ImageHandler::loadFile(char* filename)
{
	cout << "Loading file: " << filename << endl;

        // Load image.
        if (_img)
                cvReleaseImage(&_img);
        _img = cvLoadImage(filename);
        if (!_img)
        {
                cerr << "Could not load file " << filename << endl;
                exit(1);
        }
	_filename = filename;

        // Find any previous masks.
        if (_mask)
                cvReleaseImage(&_mask);
        char maskfilename[BUFSIZE];
        snprintf(maskfilename, BUFSIZE, "%s_mask.png", _filename);
        _mask = cvLoadImage(maskfilename);

        // If none, initialize new mask.
	if (!_mask)
	{
        	_mask = cvCreateImage(cvGetSize(_img), IPL_DEPTH_8U, 3);
		cvSet(_mask,cvScalar(0,0,0));
	}

	calcWindow();
}

/* save the mask for the current file to <filename>_mask.png */
void ImageHandler::save()
{
	char maskfilename[BUFSIZE];
        snprintf(maskfilename, BUFSIZE, "%s_mask.png", _filename);
        cvSaveImage(maskfilename,_mask);
	cout << "Saved mask for " << _filename << " to " << maskfilename << endl;
}

/* Catch clicks from the gui, and redirect them to the currently selected drawing tool.*/
void ImageHandler::clicked(int x, int y, int maxx, int maxy)
{
	cout << "<click>" << endl;

	switch (_curTool)
	{
		case (polygon):
			polygonTool(x,y);
		break;
		case (fill):
		{
			cout << "Filling .. " << endl;
			saveUndo();
			CvScalar seedColor = cvGet2D(_img, y, x);
			fillTool(x,y,&seedColor); // pass the color of the seed-pixel.
			calcWindow();
		}
		break;
		default:
			cerr << "Tool not implemented yet" << endl;
		break;
	}	
}

/* Switch between tools */
void ImageHandler::switchTool()
{
	if (_curTool == polygon)
		_curTool = fill;
	else
		_curTool = polygon;
	cout << "Switched to tool " << _curTool << endl;
}

/* return the window image-buffer */
IplImage* ImageHandler::getWindow()
{
	if (_small)
	{
		cvResize(_window,_small);	
		return _small;
	}
	return _window;
}

/* Request that all window image-buffers are returned at a certain size */
void ImageHandler::setWindowSize(int x, int y)
{
	_small = cvCreateImage(cvSize(x,y),IPL_DEPTH_8U, 3);
}

/* Change the color-difference threshold for the fill-tool*/
void ImageHandler::changeThreshold(int diff)
{
	int fillThreshold = sqrt(_sqFillThreshold) + diff;	
	_sqFillThreshold = pow(fillThreshold,2);
	cout << "Changed color threshold to: " << fillThreshold << endl;
}

/* Tools ------------------------------------------------------------------- */
/* The polygon-tool retrieves four consecutive clicks, and draws a filled polygon
 * between the four locations*/
void ImageHandler::polygonTool(int x, int y)
{
	_polygonClick[_polygonClickCt++]= cvPoint(x,y); 

	// After the fourth click, fill the polygon.
	if (_polygonClickCt == 4)
        {
		_polygonClickCt = 0;
		saveUndo();
		cvFillConvexPoly(_mask,_polygonClick,4,CV_RGB(255,255,255));	
		calcWindow();
        }
}

/* recursively fill all connected pixels that have a similar color to the initial pixel.*/
void ImageHandler::fillTool(int x, int y, CvScalar* seedColor)
{
	// exit conditions.
	if ((x < 0) || (x >= _mask->width))
		return; // x out of range. 
	if ((y < 0) || (y >= _mask->height))
		return; // y out of range.
	if (cvGet2D(_mask, y, x).val[0])
		return; // this region has already been visited.


	// Calculate squared distance to color.
	// sqrt is an expensive operation, so we avoid this by using a squared threshold.
	CvScalar ourColor = cvGet2D(_img, y, x);
	double sqdistance = 0;
	for (int i=0; i<3; ++i)
		sqdistance += pow((ourColor.val[i] - seedColor->val[i]),2);
 
	if (sqdistance < _sqFillThreshold)
	{
		cvSet2D(_mask, y, x, cvScalar(255,255,255));
		fillTool(x+1,y,seedColor);
		fillTool(x,y+1,seedColor);
		fillTool(x-1,y,seedColor);
		fillTool(x,y-1,seedColor);
	}
}

/* undo a single change */
void ImageHandler::undo()
{
	if (!_previous)
		return;

	cvReleaseImage(&_mask);
	_mask = _previous;
	_previous = NULL;	
	calcWindow();
}

/* Support Functions ------------------------------------------------------- */
void ImageHandler::init()
{
	_img = NULL;
	_mask = NULL;
	_window = NULL;
	_curTool = polygon;
	_small = NULL;
	_previous = NULL;

	_sqFillThreshold = 64;
	_polygonClickCt = 0;
	_polygonClick = new CvPoint[4];

	
}

/* re-calculate the window buffer*/
void ImageHandler::calcWindow()
{	if (!_window)
		_window = cvCreateImage(cvGetSize(_img), IPL_DEPTH_8U, 3);
	/* If the size of the image has changed, allocate a new window buffer.*/
	if ((cvGetSize(_window).width != cvGetSize(_img).width) || (cvGetSize(_window).height != cvGetSize(_img).height))
	{
		cvReleaseImage(&_window);
		_window = cvCreateImage(cvGetSize(_img), IPL_DEPTH_8U, 3);
	}  
	
	cvAddWeighted(_img,0.75,_mask,0.25,0,_window);	
}

/* save the old mask before changing it for undo purposes.*/
void ImageHandler::saveUndo()
{
	if (_previous)
		cvReleaseImage(&_previous);	
	_previous = cvCloneImage(_mask);
}
