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
 * The Original Code is trainer.cpp.
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
#include <deque>
#include <opencv/highgui.h>
#include "lib/histogramtool/histogramTool.h"
#include "OpenSURF/surflib.h"
#include "SignHandler.h"
#include "CornerFinder.h"
#include "modules/TestHandler.h"

#define SHOWIMAGES
#define DEBUG 

using namespace std;

int binsize = 64;
//int binsize = 32;
//const int XRES = 1600;
//const int YRES = 1200;
const int XRES = 0;
const int YRES = 0;

int _curFile=0;

CvHistogram* _posHist;
CvHistogram* _negHist;
IpVec _surfpoints;

/*
 * Train the SURF keypoints in the masked part of an image
 */
void processSingleSurf(IplImage* img, IplImage* mask)
{
	// find corners of sign.
	const int numcorners = 4;
	CvPoint corners[numcorners];
        int cornersFound = findCorners(mask,corners,numcorners,10);
	if (cornersFound != 4)
	{
		fprintf(stderr,"Could not find corners, image rejected\n");
		return;
	}
	
	// cut out the sign in a new image.
	IplImage* sign = cutSign(img,corners);

	// Calculate SURF keypoinys.
	IpVec ipts;
	surfDetDes(sign,ipts,false,4,4,2,0.0002);	
	printf("Found %d SURF keypoints\n",ipts.size());

	#ifdef SHOWIMAGES
	drawIpoints(sign, ipts);
	cvShowImage("trainer",sign);
	cvWaitKey(1);
	#endif

	// Add detected SURF keypoints to global database.
	_surfpoints.insert(_surfpoints.end(),ipts.begin(),ipts.end());

	#ifdef DEBUG
		printf("%d SURF keypoints so far.\n",_surfpoints.size());
	#endif

	cvReleaseImage(&sign);
}

/* Detect the separate masks in the mask image, and feed them one-by-one
 * to the 'processSignleSurf' function*/
void processSurf(IplImage* img, IplImage* mask)
{
	// Detect the blobs in the mask.
	CBlobResult maskblobs = CBlobResult( mask, NULL, 0, false );
	if (maskblobs.GetNumBlobs() > 5)
		return;

	// Make masks out of each of the blobs, and pass them on.
	IplImage* newMask = cvCreateImage(cvGetSize(mask),IPL_DEPTH_8U, 1);
	 for (int i = 0; i < maskblobs.GetNumBlobs(); ++i)
	{
		cvSet(newMask,cvScalar(0,0,0,0));
		(maskblobs.GetBlob(i))->FillBlob(newMask,CV_RGB(255,255,255));
		processSingleSurf(img,newMask);
	}
	cvReleaseImage(&newMask);
}

void processFile(char* file)
{
	// See if we can find a mask for this file.
        string maskfile(file);
        IplImage* mask = cvLoadImage((maskfile+"_mask.png").c_str(),CV_LOAD_IMAGE_GRAYSCALE);
	if (!mask)
	{
		cerr << "No mask for file " << file << " skipping..\n";
		return;	
	}
	

	// Load the file.
	IplImage* img;
	img = cvLoadImage(file);
        if (!img)
        {
                cerr << "Could not load file " << file << endl;
                exit(1);
        }
	cout << "Processing " << file << endl;

	// resize	
	IplImage* _mask;
	IplImage* _img; 
	if ((XRES) && ((XRES != cvGetSize(img).width) || (YRES != cvGetSize(img).height)))
	{
		_mask = cvCreateImage(cvSize(XRES,YRES),IPL_DEPTH_8U,1);
		_img = cvCreateImage(cvSize(XRES,YRES),IPL_DEPTH_8U,3);
		cvResize(mask,_mask);
		cvResize(img,_img);
		cvReleaseImage(&img);
		cvReleaseImage(&mask);
	}
	else
	{
		_mask = mask;
		_img = img;
	}

	// Generate positive / negative histograms.
	CvHistogram* posHist = calculateHistogram(_img,_mask,binsize,1);
	cvNot(_mask,_mask);
	CvHistogram* negHist = calculateHistogram(_img,_mask,binsize,1);

	// Add histograms to whole.
	addHistogram(_posHist,posHist);
	addHistogram(_negHist,negHist);

	// Process SURF training.
	processSurf(_img, _mask);
	
	// Cleanup
	cvReleaseImage(&_img);
	cvReleaseImage(&_mask);
}
void init()
{
	int binsizes[] = {binsize,binsize};
        _posHist = cvCreateHist(2,binsizes, CV_HIST_ARRAY);
        _negHist = cvCreateHist(2,binsizes, CV_HIST_ARRAY);
	cvClearHist(_posHist);
	cvClearHist(_negHist);

	#ifdef SHOWIMAGES
	cvNamedWindow("trainer");
	#endif // SHOWIMAGES
}

#ifdef DEBUG
void testSave()
{
	IpVec test;
	test = loadIpVec("_surfkeys.dat");

	assert(test.size() == _surfpoints.size());
	for (int i=0; i<test.size(); ++i)
	{
		for (int j=0; j<64; ++j)
			assert(fabs(test[i].descriptor[j] - _surfpoints[i].descriptor[j]) < 0.001);
		if ((i % 100) == 0)
			printf("%d ok\n",i);
	}
}
#endif

void cleanup()
{
	// save the results to files.
	string filename = "_posHist.hist";
	writeHistogram(_posHist,(char*) filename.c_str());
	filename = "_negHist.hist";
	writeHistogram(_negHist,(char*) filename.c_str());
	saveIpVec("_surfkeys.dat",_surfpoints);

	#ifdef DEBUG
	testSave();
	#endif

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
	{
		cvWaitKey(50);
        	processFile(argv[_curFile]);
	}

	cleanup();

	cvWaitKey(1000);
	return 0;
}
