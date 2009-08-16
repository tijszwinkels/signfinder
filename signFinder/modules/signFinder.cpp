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
 * The Original Code is signFinder.cpp .
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
#include "SignFinder.h"
#include "modules/TestHandler.h"
#include "modules/CornerFinder.h"
#include "modules/SignHandler.h"
#include "modules/OCRWrapper.h"
#include "OpenSURF/surflib.h"

using namespace std;

//#define SURF

/** Constructor */
SignFinder::SignFinder()
{
	init();
}

/** Destructor */
SignFinder::~SignFinder()
{
	cleanup();
}

/**
 *  Filter Blobs based on statistics compared to other street-signs.
 */
CBlobResult SignFinder::classifyBlobs(CBlobResult& blobs, char* file, CvSize size, IplImage* img)
{
	CBlobResult result;	

	// Pre-filtering
	// Surface > 1/600th image surface.
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, (size.width * size.height) / 600 );
	
	// Blobs not in contact with sides of image. 
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMinX(), B_EQUAL, 0);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMaxX(), B_EQUAL, size.width-1);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMinY(), B_EQUAL, 0);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMaxY(), B_EQUAL, size.height-1);

	// Iterate through the blobs, and accept or reject them based on statistical features.
	CBlob* currentBlob = NULL;
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
        	currentBlob = blobs.GetBlob(i);
	
		CBlobGetRoughness rn;
		CBlobGetElongation el;
		// ellipse
		CBlobGetMajorAxisLength ma;
		CBlobGetMinorAxisLength mi;
		CBlobGetOrientation ori;
		CBlobGetAxisRatio ar;

		double area = currentBlob->Area();
		double roughness = rn(*currentBlob);
		double diffX = currentBlob->MaxX() - currentBlob->MinX();
		double diffY = currentBlob->MaxY() - currentBlob->MinY();
		double width = mi(*currentBlob);
		double height = ma(*currentBlob);
		double XYratio = diffX / diffY; // > 1 means wider than long along the x-axis.
		double WHratio = ar(*currentBlob); 
		double orientation = ori(*currentBlob);
		double squareness = area / (width * height);
		
		if (_debug)
			fprintf	(stderr, "Blob %d - area: %f, width x height: %fx%f, orientation:%f, x/y ratio: %f, w/h ratio: %f, rougness: %f, squareness: %f",
			 i,area,width,height,orientation,XYratio,WHratio,roughness,squareness);	

		// Classify
		if ((squareness < 0.70) || (XYratio < 0.45) || WHratio < 2.5) 
		{
				
			if (_debug) cerr << "  Rejected\n";
			// make rejected blobs black.
			if (img)
				currentBlob->FillBlob(img,CV_RGB(0,0,0));

		}
		else
		{
			if (_debug) cout << "  Accepted\n";
			result.AddBlob(currentBlob);
		}

	}
	
	// Compare with labeled known-correct.
	int fp=0, fn=0, multdetect = 0;
	bool success = checkLabeledBlobs(result,size,file,fp,fn,multdetect);//,&correct,&incorrect);
	//for (int i = 0; i < correct.GetNumBlobs(); ++i )
	//	fillConvexHull(img,correct.GetBlob(i),CV_RGB(0,255,0));
	//for (int i = 0; i < incorrect.GetNumBlobs(); ++i )
	//	fillConvexHull(img,incorrect.GetBlob(i),CV_RGB(255,0,0));
	//
	if (success)
	{
		if (_showPerformance) fprintf(stderr,"For this image, we encountered %d false positives, %d undetected signs, and %d multiple detections\n",fp,fn,multdetect);
		_detperf._imagesChecked++;
		_detperf._fp += fp;
		_detperf._fn += fn;
		_detperf._multDetect += multdetect;
		if (fp || fn || multdetect)
			_detperf._imagesErr++;
	}	


	return result;
}

/**
 * Generate SURF keypoints over image, and compare them with a trained
 * database of surf keypoints. Disabled by default.
 */
void SignFinder::processSurf(IplImage* img)
{
	// Detect SURF points in image.
	IpVec ipts;
	surfDetDes(img,ipts,false,4,4,2,0.00005);
	
	// Match surf points against trained sign database.
	IpPairVec match;
	getMatches(ipts,_surfpoints,match);

	// draw matches on the image.
	for (unsigned int i=0; i<match.size(); ++i)
		drawPoint(img,match[i].first);
	
}

/* readSign support functions */

/**
 * Resize the image if necessary, and handle lifetime transparantly
 */
IplImage* SignFinder::resize(IplImage* _img)
{
	IplImage* img;
	if ((XRES) && ((XRES != cvGetSize(_img).width) || (YRES != cvGetSize(_img).height)))
        {
                img = cvCreateImage(cvSize(XRES,YRES),IPL_DEPTH_8U,3);
                cvResize(_img,img);
                cvReleaseImage(&_img);
        }
        else
                img = _img;
	
	return img;
}

/** 
 * Perform per-pixel histogram matching.
 * matched with histogram that's trained on street-signs.
 * @return binary mask, white for the pixels that matched.
 */
IplImage* SignFinder::histMatch(IplImage* img, IplImage* vis)
{
	// Perform histogram matching.
	CvMat* imgMat = cvCreateMatHeader(img->height, img->width,CV_8UC3);
        imgMat = cvGetMat(img,imgMat);
        IplImage* histMatched = skinDetectBayes(imgMat,_posHist,_negHist,_histThreshold);
	cvReleaseMatHeader(&imgMat);

	// Increase robustness for 'holes' in masks by dilating and eroding.
	//cvDilate(histMatched,histMatched,NULL,1);
	//cvErode(histMatched,histMatched,NULL,1);

	// Create visualisation of histogram matched area's if requested.
	if (vis)
	{
		cvSet(vis,cvScalar(0,0,0));
		cvCopy(img,vis,histMatched);
		cvAddWeighted(vis, 0.90, img, 0.10, 0, vis);
	} 	

	return histMatched;
}

/** 
 * Fill all blobs with a color in the image
 */
void colorBlobs(CBlobResult& blobs, IplImage* vis)
{
	CBlob* currentBlob = NULL;
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
		int rd = (i+1 & 1) * 255;
                int g = (i+1 & 2) * 127;
                int b = (i+1 & 4) * 63;
        	currentBlob = blobs.GetBlob(i);
		currentBlob->FillBlob(vis,CV_RGB(rd,g,b));
	}
}

/**
 * Accepts an image containing one of more streetsigns. 
 * Tries to segment and read (OCR) this streetsign.
 * This is the entry-funtion on the class.
 * @return newline-separated list of text on streetsigns.
 */
string SignFinder::readSigns(char* file, IplImage* result)
{
	// Load image file.
	IplImage* img = cvLoadImage(file);
        if (!img)
        {
                cerr << "Could not load file " << file << endl;
                exit(1);
        }
	if (_debug) cerr << "Processing " << file << endl;

	// Resize master if requested.
	img = resize(img);	

	// Create copy of original image that algorithms can use to draw their results on.
	bool wantsresult = true;
	if (!result)
	{
		result = cvCreateImage(cvSize(img->width,img->height),IPL_DEPTH_8U,3);
		wantsresult = false;
	}
	cvCopy(img,result);

		// return mask of pixels that are blue.
	IplImage* histMatchVis = NULL;
	if (_debug)
		histMatchVis = cvCreateImage(cvSize(img->width,img->height),IPL_DEPTH_8U,3);
	IplImage* histMatched = histMatch(img,histMatchVis);

	// Save histogram-matching visualization if requested.
	if (histMatchVis)
	{
		string matchedfile(file);
        	cvSaveImage((matchedfile+"_matched.jpg").c_str(),histMatchVis);
	}


	// Perform SURF feature-point detection for features that were detected in the trainset.
	#ifdef SURF
	processSurf(result);
	#endif	

	// Perform blob detection on the histogram matched result, and accept or reject them based on 
	// statistics.
	CBlobResult blobs = CBlobResult( histMatched, NULL, 0, false );
	blobs = classifyBlobs(blobs, file, cvSize(img->width, img->height), histMatchVis);
	if (_debug)
		cerr << "Classification: I think there are " << blobs.GetNumBlobs()  << " blue signs in this image" << endl << endl;
	
	// Color the blobs
	if (_debug)
		colorBlobs(blobs,histMatchVis);	

	// Iterate through the found streetsigns.
	int prevY = 0;
	if (result)
		prevY = result->height-1;
	CBlob* currentBlob = NULL;
	string resultText;
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
		currentBlob = blobs.GetBlob(i);

		// calculate some needed statistics over the blob
		CBlobGetMajorAxisLength ma;
		double height = ma(currentBlob);

		// Find the corners with a distance-threshold between corners of 0.75* the height.
		int numcorners = 4;
		CvPoint corners[numcorners];
		findCorners(*currentBlob,corners,numcorners,height*0.75);

		// Cut the image out with perspective correction.
		IplImage* cut = cutSign(result, corners, 4, true );

		// OCR sign, and generate performance metrics.
		string text = extractText(cut,_posHist, _negHist);	
		resultText = text + "\n";
		if (_debug)
			cerr << "---------------- Reading streetsign: " << text << endl;
		int distance = compareText(text,file);
		if (distance != -1)
		{
			if (_showPerformance)
				cout << "OCR distance to truth: " << distance << endl;
			_ocrperf._signsChecked++;
			if (distance == 0)
				_ocrperf._OCRcorrect++;
			else
				_ocrperf._editDist += distance;
		}
	
		// Add the sign to the bottom of the image.
		if (result)
		{
			prevY -= cut->height;		
			cvSetImageROI(result,cvRect(0,prevY,cut->width,cut->height));
			cvCopy(cut,result);
			cvResetImageROI(result);
			cvRectangle(result,cvPoint(0,prevY),cvPoint(cut->width,prevY+cut->height),CV_RGB(0,0,0),2);	
			// Add the text of the sign.
			drawText(result,corners[3].x + 10, corners[3].y, text);
		}
		cvReleaseImage(&cut);
	
		// Draw a convex hull around found images
		if (result)
		{	
			// get the convex hull
			CvSeq* hull;
			currentBlob->GetConvexHull(&hull);

			// Draw the convex hull
			int rd = (i+1 & 1) * 255;
			int g = (i+1 & 2) * 127;
			int b = (i+1 & 4) * 63;

			CvPoint pt0 = **CV_GET_SEQ_ELEM( CvPoint*, hull, hull->total - 1 );
			for (int j=0; j< hull->total; ++j)
			{
				CvPoint pt1 = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
				//cout << "Drawing line from " << pt0.x << "," << pt0.y << " to " << pt0.x << "," << pt0.y << endl;
				if (histMatchVis) cvLine( histMatchVis, pt0, pt1, CV_RGB(rd,g,b), 1, CV_AA, 0 );	
				cvLine( result, pt0, pt1, CV_RGB(rd,g,b), 3, CV_AA, 0 );	
				pt0 = pt1;
			}
		}
	}

	// Cleanup
	cvReleaseImage(&img);
	cvReleaseImage(&histMatched);
	if (histMatchVis)
		cvReleaseImage(&histMatchVis);
	if (!wantsresult)
		cvReleaseImage(&result);

	return resultText;
}
/**
 * Load the color histograms, required for histogram matching
 */
void SignFinder::loadHistograms()
{
	_posHist = loadHistogram("posHist.hist");
        _negHist = loadHistogram("negHist.hist");
        if (!(_posHist && _negHist))
        {
                cerr << "ERROR: posHist.hist and/or negHist.hist histogram failed to load." << endl;
                exit(1);
        }
}

/**
 * Load the SURF keypoints. Required for keypoint matching.
 */
void SignFinder::loadSurf()
{
	_surfpoints = loadIpVec("surfkeys.dat");
        fprintf(stderr,"loaded database of %d surf-points\n",_surfpoints.size());
}

/**
 * Initialize the class. Used by constructor.
 */
void SignFinder::init()
{
	_histThreshold = 0.19;
	XRES = 1600; YRES = 1200;
	_debug = false;
	_showPerformance = true;

	loadHistograms();
	#ifdef SURF
	loadSurf();
	#endif	
}

/**
 * If there are truth-grounded 'labels' available (<file>_mask.png for sign-detection and <file>.txt for OCR),
 * the software collects performance metrics. This function prints the aggregated performance metrics
 * to the screen.
 */
void SignFinder::performanceMeasurements()
{
	if (_detperf._imagesChecked)
	{
		printf("\n------------ Sign Detection performance:\n");
		printf("In %d images we encountered %d false positives, %d false negatives, and %d multiple detections\n",_detperf._imagesChecked,_detperf._fp,_detperf._fn,_detperf._multDetect);
		printf("%f %% of all images was processed correctly in its entirety\n", 100 - (((double)_detperf._imagesErr /(double) _detperf._imagesChecked)) * 100.);
	}
	if (_ocrperf._signsChecked)
	{
		printf("%d out of %d signs, which is %f %%, was OCRed entirely correctly\n",_ocrperf._OCRcorrect,_ocrperf._signsChecked, (((double)_ocrperf._OCRcorrect /(double) _ocrperf._signsChecked)) * 100.);
		printf ("Average edit distance to correct label: %f\n",_ocrperf._editDist / (double) _ocrperf._signsChecked);
	}
}

/*
 * Cleanup / free memory used in the Object.
 * Used by the destructor
 */
void SignFinder::cleanup()
{
	cvReleaseHist(&_posHist);
	cvReleaseHist(&_negHist);
	_posHist = NULL;
	_negHist = NULL;

	if (_showPerformance)
		performanceMeasurements();
}

