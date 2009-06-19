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
#include "lib/histogramtool/histogramTool.h"
#include "lib/bloblib/Blob.h"
#include "lib/bloblib/BlobResult.h"
//#include "TestHandler.h"

using namespace std;

const double HISTTHRESHOLD = 0.2;
const int WINDOWX = 1024;
const int WINDOWY = 768;

int _curFile=0;

CvHistogram* _posHist;
CvHistogram* _negHist;

#if 0 // Work In Progress
bool isLabeledBlob(CBlob& currentBlob, char* file)
{
	// See if we can find a mask for this file.
	string maskfile(file);
	IplImage* label = cvLoadImage((maskfile+"_mask.png").c_str());
	if (label)	
	{
		// We want to fill a convex hull here.
		//IplImage classification
		// Fill a mask for the current blob
		currentBlob->FillBlob(img,CV_RGB(0,0,0));

		cvReleaseImage(&label);
	}
	else
		return false;
}
#endif

/* Filter Blobs*/
CBlobResult classifyBlobs(CBlobResult& blobs, IplImage* img, char* file)
{
	CBlobResult result;	

	// Pre-filtering
	// Surface > 1/400th image surface.
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, (img->height*img->width) / 400 );
	
	// Blobs not in contact with sides of image. 
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMinX(), B_EQUAL, 0);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMaxX(), B_EQUAL, img->width-1);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMinY(), B_EQUAL, 0);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetMaxY(), B_EQUAL, img->height-1);

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
		
		printf	("Blob %d - area: %f, width x height: %fx%f, orientation:%f, x/y ratio: %f, w/h ratio: %f, rougness: %f, squareness: %f",
			 i,area,width,height,orientation,XYratio,WHratio,roughness,squareness);	

		// Classify
		if ((squareness < 0.70) || (XYratio < 0.45) || WHratio < 3.) 
		{
			cout << "  Rejected\n";
			// make rejected blobs black.
			currentBlob->FillBlob(img,CV_RGB(0,0,0));

		}
		else
		{
			cout << "  Accepted\n";
			result.AddBlob(currentBlob);
		}
	}

	return result;
}

void processFile(char* file)
{
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
	IplImage* histMatched = skinDetectBayes(imgMat,_posHist,_negHist,HISTTHRESHOLD); 

	// Increase robustness for 'holes' in masks by dilating and eroding.
	//cvDilate(histMatched,histMatched,NULL,1);
	//cvErode(histMatched,histMatched,NULL,1);

	IplImage* overlay = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 3);
	cvSet(overlay,cvScalar(0,0,0));

	IplImage* result = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 3); // result image.
	//cvCvtColor(histMatched, overlay, CV_GRAY2RGB);
	cvCopy(img,overlay,histMatched);
	cvCopy(img,result);
	// blend original image wit result: Keep all pixels in the mask the same, make the rest darker. 
	cvAddWeighted(overlay, 0.90, img, 0.10, 0, overlay);

	// Save histogram-matched image.
	string matchedfile(file);	
	cvSaveImage((matchedfile+"_matched.jpg").c_str(),overlay);
	//cout << "Result saved as " << matchedfile+"_matched.jpg" << endl;

	// Perform blob detection.
	// invert mask
	//cvNot(histMatched,histMatched);
	/* Hoe werkt deze threshold? Ik ga er vanuit dat hij alles onder deze waarde 0 maakt.
 	 * dat lijkt te kloppen, want als ik 255 invul wordt alles zwart.
 	 * Probleem: Waarom extract hij blobs van de background, ook als ik alles inverteer.*/
	// solved: ook van de background worden blobs extracted.
	CBlobResult blobs = CBlobResult( histMatched, NULL, 0, false );
	CBlobResult oldblobs = blobs;
	blobs = classifyBlobs(blobs,overlay, file);
	//blobs.PrintBlobs("blobs.dat");

	cout << "Classification: I think there are " << blobs.GetNumBlobs()  << " blue signs in this image" << endl << endl;
	
	/*
	// get biggest blob.
	CBlob biggestBlob;
	blobs.GetNthBlob( CBlobGetArea(), 1, biggestBlob );
	biggestBlob.FillBlob(overlay, CV_RGB(0,0,255));
	*/

	CBlob* currentBlob = NULL;
	
	// Color the blobs
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
		int rd = (i+1 & 1) * 255;
                int g = (i+1 & 2) * 127;
                int b = (i+1 & 4) * 63;
        	currentBlob = blobs.GetBlob(i);
		currentBlob->FillBlob(overlay,CV_RGB(rd,g,b));
	}

	// Compute and draw the convex hull of the blobs
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
		// get the convex hull
		currentBlob = blobs.GetBlob(i);
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
			cvLine( overlay, pt0, pt1, CV_RGB(rd,g,b), 1, CV_AA, 0 );	
			cvLine( result, pt0, pt1, CV_RGB(rd,g,b), 3, CV_AA, 0 );	
			pt0 = pt1;
		}
	}
	#ifdef SHOWIMAGES
	cvShowImage("signFinder",overlay);
	cvWaitKey(0);
	#endif

	string blobfile(file);	
	cvSaveImage((matchedfile+"_blob.jpg").c_str(),overlay);
	string resultfile(file);	
	cvSaveImage((matchedfile+"_result.jpg").c_str(),result);

	// Cleanup
	cvReleaseMatHeader(&imgMat);
	cvReleaseImage(&img);
	cvReleaseImage(&overlay);
	cvReleaseImage(&histMatched);
	cvReleaseImage(&result);
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

	#ifdef SHOWIMAGES
		cvNamedWindow("signFinder",0);
        	cvResizeWindow("signFinder", WINDOWX, WINDOWY);	
	#endif
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
	cleanup();

	return 0;
}
