#include <opencv/cv.h>

double compareMasks(IplImage* estimation, IplImage* label, double* _fp = NULL);
bool blobCorrect(IplImage* blob, IplImage* label,double numBlobs);
