#ifndef SIGNHANDLER
#define SIGNHANDLER

#include <opencv/cxcore.h>
#include "CornerFinder.h"

IplImage* cutSign(IplImage* origImg, CvPoint* corners, int numcorners, bool drawcircles)
{

	// convert corners to CvPoint2D32f.
        CvPoint2D32f cornersf[numcorners];
        for (int i=0; i<numcorners; ++i)
                cornersf[i] = cvPointTo32f(corners[i]);

	printf("Corners: %d,%d %d,%d %d,%d %d,%d\n",corners[0].x,corners[0].y,corners[1].x,corners[1].y,corners[2].x,corners[2].y,corners[3].x,corners[3].y);

        // target-points shift (shift=1 for corner 0 is upper-left corner)
        int shift = 1;
        //if (corners[1].x > corners[3].x) // seems that corner[1] has skipped to the right side (and therefore 3 to the left).
        //       shift = 0;
        if (corners[3].x < corners[1].x)
		shift = 3;
	else
		shift = 2;
	printf("shift: %d\n",shift);

	// Create target-image with right size.
        double xDiffBottom = pointDist(corners[(0+shift)%4], corners[(1+shift)%4]);
        double yDiffLeft = pointDist(corners[(3+shift)%4], corners[(0+shift)%4]);
        IplImage* cut = cvCreateImage(cvSize(xDiffBottom,yDiffLeft), IPL_DEPTH_8U, 3);

	// target points for perspective correction.
        CvPoint2D32f cornerstarget[numcorners];
        cornerstarget[(0+shift)%4] = cvPoint2D32f(0,0);
        cornerstarget[(1+shift)%4] = cvPoint2D32f(cut->width-1,0);
        cornerstarget[(2+shift)%4]= cvPoint2D32f(cut->width-1,cut->height-1);
        cornerstarget[(3+shift)%4] = cvPoint2D32f(0,cut->height-1);
	printf("Corners: %f,%f %f,%f %f,%f %f,%f\n",cornerstarget[0].x,cornerstarget[0].y,cornerstarget[1].x,cornerstarget[1].y,cornerstarget[2].x,cornerstarget[2].y,cornerstarget[3].x,cornerstarget[3].y);
        
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
#endif
