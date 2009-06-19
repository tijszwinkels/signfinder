/*
File: histogramTool.h

Interface file for histogramTool

Jaldert Rombouts <rombouts@ai.rug.nl>
2008-07-02.

*/

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>

using namespace std;
//#include <iostream>

void releaseImage(IplImage* img);
void releaseHistogram(CvHistogram* hist_);

IplImage* skinDetectBayes(CvMat* input_, CvHistogram* skinHist_, CvHistogram* nonSkinHist_, float threshold_, int color_code_ = 1, CvMat* mask_ = NULL);
IplImage* skinDetectBayes(IplImage* input, CvHistogram* skinHist, CvHistogram* nonSkinHist, float threshold, int* cois, float** ranges, IplImage* mask_ = NULL);

//IplImage* skinDetectBayes(CvMat* input, CvHistogram* skinHist, CvHistogram* nonSkinHist, float threshold);
//IplImage* skinDetect(CvMat* yCrCbImg, CvHistogram* hist);

void addHistogram(CvHistogram* a, CvHistogram* b);
void writeHistogram(CvHistogram* hist, char* filename);
CvHistogram* loadHistogram(char* filename);
CvHistogram* histogram(CvMat* image, CvMat* mask, char* filename, int dims);
void printGNUplot(CvHistogram*  hist,  char* filename);
IplImage* backProject(CvMat* yCrCbImg, CvHistogram* hist);




void bgr2normalizedrgb(IplImage* src_, IplImage* dst_);
IplImage* convertCvMatToIpl(CvMat* in_, int depth_, int channels_);

CvHistogram* calculateHistogram(CvMat* in_, CvMat* mask, int dim, int code);
CvHistogram* calculateNegHistogram(CvMat* in_, CvMat* mask, int dim, int code);

CvHistogram* calculateHistogram(CvMat* in_, CvMat* mask_, int* cois_, int dim_, int code_, float** ranges);
CvHistogram* calculateHistogram(IplImage* in_, IplImage* mask_, int* cois_, int dim_, int code_, float** ranges);


