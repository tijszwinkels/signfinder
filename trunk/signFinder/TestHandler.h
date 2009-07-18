/* 
 * See .cpp file for more information
 */

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "lib/bloblib/Blob.h"
#include "lib/bloblib/BlobResult.h"


double compareMasks(IplImage* estimation, IplImage* label, double* _fp = NULL);
bool blobCorrect(IplImage* blob, IplImage* label,double numBlobs);

void fillConvexHull(IplImage* img, CvSeq* hull, CvScalar color);
void fillConvexHull(IplImage* img, CBlob* blob, CvScalar color);

bool checkLabeledBlobs(CBlobResult& detectedBlobs, IplImage* origImg, char* file, int& fp, int& fn, int& multipleDetections, CBlobResult* correctBlobsOut = NULL, CBlobResult* incorrectBlobsOut = NULL);
