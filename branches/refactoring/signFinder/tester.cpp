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
 * The Original Code is tester.cpp .
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


#include <iostream>
#include <fstream>
#include <deque>
#include <map>
#include "lib/histogramtool/histogramTool.h"
#include "TestHandler.h"
//#include "lib/bloblib/Blob.h"
//#include "lib/bloblib/BlobResult.h"

using namespace std;

double HISTTHRESHOLD = 0.2;

#define ROC
const double minThr = 0;
const double maxThr = 5;
const double measurements = 200;
const double exponent = 1.1; 

const int WINDOWX = 1600;
const int WINDOWY = 1200;

int _curFile=0;

CvHistogram* _posHist;
CvHistogram* _negHist;

struct RocColItem {
	deque<double> tp;
	deque<double> fp;
};

typedef map<double,RocColItem> RocMap;
RocMap rocmap;

void processFile(char* file)
{
	// See if we can find a mask for this file.
        string maskfile(file);
        IplImage* label = cvLoadImage((maskfile+"_mask.png").c_str());
	// We can't test performance is there's no known correct mask.
	if (!label)
	{
		cerr << "No mask for file " << file << " skipping..\n";
		return;	
	}

	IplImage* img;
	img = cvLoadImage(file);
        if (!img)
        {
                cerr << "Could not load file " << file << endl;
                exit(1);
        }
	cout << "Processing " << file << endl;
	// return mask of images that have been detected.
	CvMat* imgMat = cvCreateMatHeader(img->height, img->width,CV_8UC3);
	imgMat = cvGetMat(img,imgMat);

	IplImage* histMatched, result;
#ifdef ROC
	double minStep = (maxThr - minThr) / pow(exponent,measurements);
	for (HISTTHRESHOLD=minThr; HISTTHRESHOLD<maxThr; HISTTHRESHOLD += minStep )
	{
		minStep *= exponent;
#endif
		IplImage* histMatched = skinDetectBayes(imgMat,_posHist,_negHist,HISTTHRESHOLD); 
		IplImage* result = cvCreateImage(cvGetSize(histMatched), IPL_DEPTH_8U, 3);
		cvCvtColor(histMatched, result, CV_GRAY2RGB);

		// Compare results with known-correct mask.
		double fp;
		double tp = compareMasks(result,label,&fp);
		printf("**** file: %s,thr: %f, tp: %f, fp: %f\n",file,HISTTHRESHOLD, tp,fp);
		// Add the results to the ROC-curve.
		//rocmap[HISTTHRESHOLD].threshold = HISTTHRESHOLD;
		rocmap[HISTTHRESHOLD].tp.push_back(tp);
		rocmap[HISTTHRESHOLD].fp.push_back(fp);

	cvReleaseImage(&histMatched);
	cvReleaseImage(&result);
#ifdef ROC
	}
#endif


	// Cleanup
	cvReleaseMatHeader(&imgMat);
	cvReleaseImage(&img);
	cvReleaseImage(&label);
}

void printResults()
{
	ofstream ofs("RocCurve.dat");
	ofs << "# Histogram-matching RoC-curve based on the test-set." << endl;
	ofs << "# threshold\ttp\tsd\tfp\tsd" << endl << endl;


	// Iterate through all elements in the RoC-curve.
	RocMap::iterator end = rocmap.end();
	RocMap::iterator it = rocmap.begin();
	double surface = 0, prevTp=1, prevFp=1;
        while (it != end)
	{
		double threshold = it->first;
		RocColItem cur = it->second;

		// calculate averages
		double tpavg = 0;
		double fpavg = 0;
		for (int i=0; i<cur.tp.size(); ++i)
		{
			tpavg += cur.tp[i];
			fpavg += cur.fp[i];
		}
		tpavg /= cur.tp.size();
		fpavg /= cur.tp.size();

		// calculate standard deviation
		double tpdev = 0;
		double fpdev = 0;	
		for (int i=0; i<cur.tp.size(); ++i)
		{
			tpdev += pow(cur.tp[i] - tpavg,2);
			fpdev += pow(cur.fp[i] - fpavg,2);
		}
		tpdev = sqrt(tpdev / cur.tp.size());
		fpdev = sqrt(fpdev / cur.fp.size());

		// write results to file.
		ofs << threshold << " \t " << tpavg << " \t " << tpdev << " \t " << fpavg << " \t " << fpdev << endl;
		cout << threshold << " \t " << tpavg << " \t " << tpdev << " \t " << fpavg << " \t " << fpdev << endl;

		// Calculate the surface under the curve.
		// Warning: Assumes rising threshold / falling tp and fp.
		double dFp = prevFp - fpavg;
		double dTp = prevTp - tpavg;
		surface+= dFp * (tpavg + (0.5*dTp)); // assume linear progress between previous and current measurement.
	
		// setup for next iteration.
		prevTp = tpavg;
		prevFp = fpavg;
		++it;
	}
	// calculate the surface under the last part of the curve towards tp=0 and fp=0
	surface += prevFp * (0.5*prevTp);

	cout << "Surface under RoC curve: " << surface << endl;	
	ofs << endl << "# Surface under RoC curve: " << surface << endl;	

	ofs.close();
}

void init()
{
	_posHist = loadHistogram("posHist.hist");
        _negHist = loadHistogram("negHist.hist");
	if (!(_posHist && _negHist))
	{
		cerr << "ERROR: posHist.hist and/or negHist.hist histogram failed to load." << endl;
		exit(1);
	}
}

void cleanup()
{
	cvReleaseHist(&_posHist);
	cvReleaseHist(&_negHist);
	_posHist = NULL;
	_negHist = NULL;
}

int main(int argc, char** argv)
{
        if (argc < 2)
        {
                cerr << "Usage: " << argv[0] << " <image-files>" << endl;
                exit(0);
        }

	init();
	// iterate through all files.
	while (++_curFile < argc)
        	processFile(argv[_curFile]);

	printResults();
	cleanup();

	return 0;
}
