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


#include <iostream>
#include <opencv/highgui.h>
#include <stdlib.h>
#include "ImageHandler.h"

/*
 * A little program to generate masks for images. 
 * Heavily inspired on the 'RegionGrow' program by Jaldert Rombouts, but rewritten for functionality.
 *
 * June 2009, Tijs Zwinkels <opensource AT tumblecow DOT net>
 */


using namespace std;

/* Configuration */
const static int WINDOWX = 1024;
const static int WINDOWY = 768;
const static int BUFSIZE = 256;


/* vars */
ImageHandler ih;
int _curWinX, _curWinY;
int _argc;
char ** _argv;
int _curFile=1;


void showImage(IplImage* img)
{
	// Check for changed aspect ratio, and resize the window if necessary
	if (fabs(((double) img->width / (double)  img->height) - ((double) _curWinX / (double) _curWinY)) > 0.01)
	{
		double newAspect = img->width / (double) img->height;
		_curWinY = WINDOWX / newAspect;
		cvResizeWindow("MaskMaker", _curWinX, _curWinY);
	}
	
	cvShowImage("MaskMaker", img);
}

/* Event Handlers */
void mouseEvent(int event, int x, int y, int flags, void* param)
{
	switch(event)
	{
		case (CV_EVENT_LBUTTONDOWN):
			ih.clicked(x,y,WINDOWX,WINDOWY);
			showImage(ih.getWindow());
		break;
		
		case (CV_EVENT_RBUTTONDBLCLK):
			ih.undo();
			showImage(ih.getWindow());
		break;
	}
}



void keyEvent(int key)
{
	switch (key)
	{
		case('s'): // save 
			ih.save();
		break;
		case('q'): // quit
			exit(0);
		break;
		case('t'): // switch tool
			ih.switchTool();	
		break;
		case('z'): // undo 
			ih.undo();	
			showImage(ih.getWindow());
		break;
		case('='): // increase threshold. 
		case('+'):
			ih.changeThreshold(1);	
		break;
		case('-'): // decrese threshold. 
			ih.changeThreshold(-1);	
		break;
		case('n'): // next image-file.
			if (_curFile+1 < _argc)
			{
				ih.loadFile(_argv[++_curFile]);
				showImage(ih.getWindow());
			}
			else
				cerr << "We're at the end of the file-list" << endl;
		break;
		case('p'): // previous image-file.
			if (_curFile-1 > 0)
			{
				ih.loadFile(_argv[--_curFile]);
				showImage(ih.getWindow());
			}
			else
				cerr << "We're at the beginning of the file-list" << endl;
		break;
	}
}

void init()
{
	// Create window, and set mouse callback.
	cvNamedWindow("MaskMaker",0);
	_curWinX = WINDOWX;
	_curWinY = WINDOWY;
	cvResizeWindow("MaskMaker", _curWinX, _curWinY);
	cvSetMouseCallback("MaskMaker", &mouseEvent, NULL);
}

void mainloop()
{
	while(true)
		keyEvent(cvWaitKey(100));
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		cerr << "Usage: " << argv[0] << " <image-files>" << endl;
		exit(0);
	}
	_argc = argc;
	_argv = argv;
	
	//int key = cvWaitKey(100);	
	init();
	ih.loadFile(argv[1]);
	showImage(ih.getWindow());

	mainloop();

	return 0;
}
