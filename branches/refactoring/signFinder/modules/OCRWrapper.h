#include<string>
#include<opencv/cv.h>
using namespace std;

string extractText(IplImage* sign, CvHistogram* _posHist, CvHistogram* _negHist);
