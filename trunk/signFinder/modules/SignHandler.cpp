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
 * The Original Code is SignHandler.cpp .
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

#ifndef SIGNHANDLER
#define SIGNHANDLER

#include <opencv/cxcore.h>
#include "CornerFinder.h"
#include "SignHandler.h"

const bool _debug = false;

IplImage* cutSign(IplImage* origImg, CvPoint* corners, int numcorners, bool drawcircles)
{

	// convert corners to CvPoint2D32f.
        CvPoint2D32f cornersf[numcorners];
        for (int i=0; i<numcorners; ++i)
                cornersf[i] = cvPointTo32f(corners[i]);

	if (_debug) printf("Corners: %d,%d %d,%d %d,%d %d,%d\n",corners[0].x,corners[0].y,corners[1].x,corners[1].y,corners[2].x,corners[2].y,corners[3].x,corners[3].y);

	// Create target-image with right size.
        double xDiffBottom = pointDist(corners[0], corners[1]);
        double yDiffLeft = pointDist(corners[0], corners[3]);
        IplImage* cut = cvCreateImage(cvSize(xDiffBottom,yDiffLeft), IPL_DEPTH_8U, 3);

	// target points for perspective correction.
        CvPoint2D32f cornerstarget[numcorners];
        cornerstarget[0] = cvPoint2D32f(0,0);
        cornerstarget[1] = cvPoint2D32f(cut->width-1,0);
        cornerstarget[2]= cvPoint2D32f(cut->width-1,cut->height-1);
        cornerstarget[3] = cvPoint2D32f(0,cut->height-1);
	if (_debug) printf("Corners: %f,%f %f,%f %f,%f %f,%f\n",cornerstarget[0].x,cornerstarget[0].y,cornerstarget[1].x,cornerstarget[1].y,cornerstarget[2].x,cornerstarget[2].y,cornerstarget[3].x,cornerstarget[3].y);
        
	// Apply perspective correction to the image.
        CvMat* transmat = cvCreateMat(3, 3, CV_32FC1); // Colums, rows ?
        transmat = cvGetPerspectiveTransform(cornersf,cornerstarget,transmat);
        cvWarpPerspective(origImg,cut,transmat);
        cvReleaseMat(&transmat);

	// Draw yellow circles around the corners.
	if (drawcircles)
		for (int i=0; i<numcorners; ++i)
			cvCircle(origImg, corners[i],5,CV_RGB(255,255,0),2);

        return cut;
}

void drawText(IplImage* img, int x, int y, string text)
{
         CvFont fnt;
         cvInitFont(&fnt,/*CV_FONT_HERSHEY_SIMPLEX*/CV_FONT_VECTOR0,2,2,0,5);

         string remainingText = text;

         if ((remainingText.find("\n") == string::npos))
		remainingText += "\n";

         while ((remainingText.find("\n") != string::npos))
         {
                size_t loc = remainingText.find("\n");
                string putText = remainingText.substr(0,loc);
                remainingText = remainingText.substr(loc+1);

                cvPutText(img,putText.c_str(),cvPoint(x,y),&fnt,CV_RGB(1,1,1));

                CvSize size;
                int belowBase;
                cvGetTextSize(putText.c_str(),&fnt,&size,&belowBase);
                y+=size.height - belowBase + 40;
         }
}
#endif
