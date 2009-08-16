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
 * The Original Code is CornerFinder.cpp .
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


#include <stdio.h>
#include <iostream>
#include <math.h>
#include "CornerFinder.h"
#include "TestHandler.h"

const bool _debug = false;

using namespace std;

int findCorners(IplImage* maskImg, CvPoint* corners, int numCorners, double distThr)
{
	CvPoint2D32f f_corners[numCorners];
	memset(corners,0,sizeof(CvPoint) * numCorners);

	// Extract the corners.
	IplImage* eigtmp = cvCreateImage(cvSize(maskImg->width, maskImg->height),IPL_DEPTH_32F,1);
	IplImage* tmp2 = cvCreateImage(cvSize(maskImg->width, maskImg->height),IPL_DEPTH_32F,1);
	int cornersFound = numCorners;
	cvGoodFeaturesToTrack(maskImg,eigtmp,tmp2,f_corners,&cornersFound, 0.1,distThr,NULL,9);
	if (_debug) cerr << "findCorners:: Found " << cornersFound << " Corners." << endl;

	//Cleanup
	cvReleaseImage(&tmp2);
	cvReleaseImage(&eigtmp);

	// Find the upper left point.
	double mindist = 1000000000;
	int mindisti = -1;
	CvPoint2D32f upperLeft = cvPoint2D32f(0,0);
	//CvPoint2D32f upperLeft = cvPoint2D32f(maskImg->width, maskImg->height);
	for (int i=0; i<numCorners; ++i)
		if (pointDist(upperLeft, f_corners[i]) < mindist)
		{
			mindist = pointDist(upperLeft, f_corners[i]);
			mindisti = i;
			//printf("new minimum distance: %f,%d\n",mindist, mindisti);
		}

	// Convert the detected corners to CvPoints.
	// make sure that the upper left corner is at the first position.
	CvPoint foundcorners[numCorners];
	for (int i=0; i<numCorners; ++i)
		foundcorners[i] = cvPointFrom32f(f_corners[(i+mindisti) % numCorners]);

	// create a convex hull of the points.
	CvMemStorage* storage = cvCreateMemStorage();
        CvSeq* ptseq = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour), sizeof(CvPoint), storage );
        for(int i = 0; i < numCorners; i++ )
		cvSeqPush(ptseq,&(foundcorners[i]));
	CvSeq* hull = cvConvexHull2( ptseq, 0, CV_COUNTER_CLOCKWISE, 0 );

	// order the points 
	int corneri = 0;
        for (int j=0; j< hull->total; ++j) //iterate through hullpoints.
	{
        	CvPoint pt = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
		for (int corner=0; corner < numCorners; ++corner) //iterate through found corners.
		{
			CvPoint hullpt = findClosestConvexHullPoint(foundcorners[corner],hull);
			if ((pt.x == hullpt.x) && (pt.y == hullpt.y))
				corners[corneri++] = hullpt; // Snap corner to convex hull.
		}
	}

	// Cleanup
	cvClearMemStorage( storage );
	return cornersFound;
}

/* Find the corners of the convex hull around the blob with the cvGoodFeaturesToTrack function*/
int findCorners(CBlob& blob, CvPoint* corners, int numCorners, double distThr)
{
	// Fill the convex hull around the blob.
	IplImage* fill = cvCreateImage(cvSize(blob.MaxX()+10, blob.MaxY()+10),IPL_DEPTH_8U,1);
	fillConvexHull(fill,&blob,cvScalar(255,0,0,0));	

	int foundcorners = findCorners(fill, corners, numCorners, distThr);
	cvReleaseImage(&fill);
	return foundcorners;
}

/* Find corners by finding the points exceeding the angle-threshold in the convex hull*/
void findCorners_method1(CBlob& blob, CvPoint* corners, int numCorners, double distThr, double angleThr )
{
	int cornIndex = 0;

	// Initialize the corner-array.
	memset(corners,0,sizeof(CvPoint) * numCorners);

	// Iterate through all points of the convex hull around the blob.
        CvSeq* hull;
        blob.GetConvexHull(&hull);
        CvPoint pt0 = **CV_GET_SEQ_ELEM( CvPoint*, hull, hull->total - 1 );
	CvPoint prevCorner;
	double prevDirection = -99;
        for (int j=0; j< hull->total; ++j)
        {
                CvPoint pt1 = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
		
		// Compare current line-direction with previous line-direction.
		double direction = atan((double)(pt0.x - pt1.x)/(double) (pt0.y - pt1.y));
		if (prevDirection != -99)
		{
			double deltaDirection = fabs(direction - prevDirection);
			//cout << "deltaDirection: " << deltaDirection << "/" << angleThr << " pointDist: " << pointDist(pt0,prevCorner) << "/"  << distThr  << endl;
			if ( (deltaDirection > angleThr) && (pointDist(pt0,prevCorner) > distThr) )
			{
				if (cornIndex > (numCorners -1))
				{
					cerr << "findCorners:: found too many corners" << endl;
					return;
				}
				// found a corner, add it to the list.
				printf("Found corner %d at (%d,%d).\n",cornIndex,pt0.x,pt0.y);
				corners[cornIndex++] = pt0;
				prevCorner = pt0;
			}
				
		}
                
		// Set-up next iteration.
		pt0 = pt1;
		prevDirection = direction;
        }
}	

/* Support Functions */
double pointDist(CvPoint& p0, CvPoint& p1)
{
	return sqrt(pow(p0.x-p1.x,2)+pow(p0.y-p1.y,2));	
}

double pointDist(CvPoint2D32f& p0, CvPoint2D32f& p1)
{
	return sqrt(pow(p0.x-p1.x,2)+pow(p0.y-p1.y,2));
}

/* Return the point in the convex hull, that is closest to the given point */
CvPoint findClosestConvexHullPoint(CvPoint corner, CvSeq* hull)
{
	CvPoint curClosest = cvPoint(1000000,1000000);
	for (int j=0; j< hull->total; ++j)
	{
        	CvPoint hullpt = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
		if (pointDist(hullpt,corner) < pointDist(curClosest,corner))
			curClosest = hullpt;
	}
	return curClosest;
	 
}
