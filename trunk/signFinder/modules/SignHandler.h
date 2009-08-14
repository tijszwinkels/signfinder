#include<string>

using namespace std;

IplImage* cutSign(IplImage* origImg, CvPoint* corners,int numcorners=4, bool drawcircles=false);
void drawText(IplImage* img, int x, int y, string text);

