#include <opencv/cv.h>
#include "lib/bloblib/Blob.h"
#include "lib/bloblib/BlobResult.h"

int findCorners(CBlob& blob, CvPoint* corners, int numCorners = 4, double distThr = 10);
int findCorners(IplImage* maskImage, CvPoint* corners, int numCorners = 4, double distThr = 10);

/* Support */
CvPoint findClosestConvexHullPoint(CvPoint corner, CvSeq* hull);
double pointDist(CvPoint& p0, CvPoint& p1);
double pointDist(CvPoint2D32f& p0, CvPoint2D32f& p1);
