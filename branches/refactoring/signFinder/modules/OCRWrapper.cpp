#include "OCRWrapper.h"
#include "../lib/histogramtool/histogramTool.h"
#include <opencv/highgui.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>

string extractText(IplImage* sign, CvHistogram* _posHist, CvHistogram* _negHist)
{
	// Use the histogram matcher to segment the letters for the OCR.
	CvMat* imgMat = cvCreateMatHeader(sign->height, sign->width,CV_8UC3);
        imgMat = cvGetMat(sign,imgMat);
        IplImage* histMatched = skinDetectBayes(imgMat,_posHist,_negHist,0.2);
        cvNot(histMatched,histMatched);

	// save the image for tesseract + cleanup
	cvSaveImage("OCRsign.tiff",histMatched);
	cvReleaseImage(&histMatched);
	cvReleaseMat(&imgMat);

	// Crunch the image through tesseract, and gather the results.
	system("tesseract OCRsign.tiff OCR");
	string result;
	result[0] = NULL;
	ifstream ifs("OCR.txt");
	if (!ifs.bad())
		getline(ifs, result);
	else
		cerr << "WARNING: Could not read OCR results. Is tesseract properly installed?" << endl;
	
	// Cleanup
	remove("OCRsign.tiff");
	remove("OCR.txt");
	remove("OCR.raw");
	remove("OCR.map");
	
	return result;
}
