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
#include "lib/HierarchicalStore/HierarchicalStore.h"
#include "modules/TestHandler.h"
#include "modules/CornerFinder.h"
#include "modules/SignHandler.h"
#include "modules/OCRWrapper.h"
#include "OpenSURF/surflib.h"

using namespace std;

//#define SHOWIMAGES
//#define SURF

const double HISTTHRESHOLD = 0.19;
const int WINDOWX = 1024;
const int WINDOWY = 768;
const int XRES = 1600;
const int YRES= 1200;
//const int XRES = 0; // Don't resize images before processing.
//const int YRES = 0;



int _curFile=0;
int _fp=0, _fn=0, _multDetect=0, _imagesChecked=0, _imagesErr=0; // performance metrics detecting images.
int _OCRcorrect=0,_signsChecked=0; double _editDist;// performance metrics OCR images.

CvHistogram* _posHist;
CvHistogram* _negHist;
IpVec _surfpoints;

/* Filter Blobs*/
CBlobResult classifyBlobs(CBlobResult& blobs, IplImage* img, char* file)
{
	CBlobResult result;	

	// Pre-filtering
	// Surface > 1/400th image surface.
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, (img->height*img->width) / 600 );
	
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
		if ((squareness < 0.70) || (XYratio < 0.45) || WHratio < 2.5) 
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
	
	// Compare with labeled.
	//CBlobResult correct, incorrect;
	int fp=0, fn=0, multdetect = 0;
	bool success = checkLabeledBlobs(result,img,file,fp,fn,multdetect);//,&correct,&incorrect);
	//for (int i = 0; i < correct.GetNumBlobs(); ++i )
	//	fillConvexHull(img,correct.GetBlob(i),CV_RGB(0,255,0));
	//for (int i = 0; i < incorrect.GetNumBlobs(); ++i )
	//	fillConvexHull(img,incorrect.GetBlob(i),CV_RGB(255,0,0));
	if (success)
	{
		printf("For this image, we encountered %d false positives, %d undetected signs, and %d multiple detections\n",fp,fn,multdetect);
		++_imagesChecked;
		_fp += fp;
		_fn += fn;
		_multDetect += multdetect;
		if (fp || fn || multdetect)
			++_imagesErr;
	}	


	return result;
}

/* If we have the perspective-corrected streetsign, save it as a picture with thresholded leters
 * for the OCR software.*/
#if 0
void processSign(IplImage* sign, char* file, int i)
{
	// Convert to greyscale
	IplImage* grey = cvCreateImage(cvGetSize(sign), IPL_DEPTH_8U, 1);
	cvCvtColor(sign,grey,CV_RGB2GRAY);

	// Use the histogram-matcher blob detector to segment the letters.
	CvMat* imgMat = cvCreateMatHeader(sign->height, sign->width,CV_8UC3);
        imgMat = cvGetMat(sign,imgMat);
        IplImage* histMatched = skinDetectBayes(imgMat,_posHist,_negHist,HISTTHRESHOLD);
	cvNot(histMatched,histMatched);

	// Save the image.
	char signfile[256];   
	snprintf(signfile, 256, "%s_sign%d.pgm",file, i );  
	//cvSaveImage(signfile,histMatched);
	cvSaveImage(signfile,sign);

	// Cleanup
	cvReleaseImage(&grey);
	cvReleaseMat(&imgMat);
	cvReleaseImage(&histMatched);
}
#endif

void processSurf(IplImage* img)
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

/* Big Ugly 'this function does everything' function. */
void processFile(char* file)
{
	CvMemStorage* mem = cvCreateMemStorage();
	IplImage* _img = cvLoadImage(file);
	//ImageElement imageElement("InputImage",file);
	//IplImage* _img = (IplImage*) imageElement.getElement();
        if (!_img)
        {
                cerr << "Could not load file " << file << endl;
                exit(1);
        }
	cout << "Processing " << file << endl;


	// Resize if requested.
	IplImage* img;
	if ((XRES) && ((XRES != cvGetSize(_img).width) || (YRES != cvGetSize(_img).height)))
        {
                img = cvCreateImage(cvSize(XRES,YRES),IPL_DEPTH_8U,3);
                cvResize(_img,img);
                cvReleaseImage(&_img);
        }
        else
                img = _img;
	//imageElement.setElement(img);
	

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
	// blend original image with overlay: Keep all pixels in the mask the same, make the rest darker. 
	cvAddWeighted(overlay, 0.90, img, 0.10, 0, overlay);

	// Save histogram-matched image.
	string matchedfile(file);	
	cvSaveImage((matchedfile+"_matched.jpg").c_str(),overlay);
	//cout << "Result saved as " << matchedfile+"_matched.jpg" << endl;

	#ifdef SURF
	// Perform SURF feature-point detection for features that were detected in the trainset.
	processSurf(result);
	#endif	

	// Perform blob detection.
	CBlobResult blobs = CBlobResult( histMatched, NULL, 0, false );
	CBlobResult oldblobs = blobs;
	blobs = classifyBlobs(blobs,overlay, file);
	//blobs.PrintBlobs("blobs.dat");

	cout << "Classification: I think there are " << blobs.GetNumBlobs()  << " blue signs in this image" << endl << endl;
	
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

	// Iterate through the found streetsigns.
	int prevY = result->height-1;
	for (int i = 0; i < blobs.GetNumBlobs(); ++i )
	{
		// Cut out the street signs.
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
		//processSurf(cut); // process SURF on the sign-image instead.
		#ifdef SHOWIMAGES
		cvShowImage("signFinder",cut);
		cvWaitKey(0);
		#endif

		// OCR sign, and generate performance metrics.
		string text = extractText(cut,_posHist, _negHist);	
		cout << "---------------- Reading streetsign: " << text << endl;
		int distance = compareText(text,file);
		if (distance != -1)
		{
			cout << "----- edit distance to label: " << distance << endl;
			++_signsChecked;
			if (distance == 0)
				++_OCRcorrect;
			else
				_editDist += distance;
		}
	
		// Add the sign to the bottom of the image.
		prevY -= cut->height;		
		cvSetImageROI(result,cvRect(0,prevY,cut->width,cut->height));
		cvCopy(cut,result);
		cvResetImageROI(result);
		cvRectangle(result,cvPoint(0,prevY),cvPoint(cut->width,prevY+cut->height),CV_RGB(0,0,0),2);	
		// Add the text of the sign.
		drawText(result,corners[3].x + 10, corners[3].y, text);

		// Cleanup
		cvReleaseImage(&cut);
		
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
			cvLine( overlay, pt0, pt1, CV_RGB(rd,g,b), 1, CV_AA, 0 );	
			cvLine( result, pt0, pt1, CV_RGB(rd,g,b), 3, CV_AA, 0 );	
			pt0 = pt1;
		}
	}
	#ifdef SHOWIMAGES
	cvShowImage("signFinder",overlay);
	cvWaitKey(0);
	cvShowImage("signFinder",result);
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
	cvReleaseMemStorage(&mem);
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
	#ifdef SURF
	_surfpoints = loadIpVec("surfkeys.dat");
	printf("loaded database of %d surf-points\n",_surfpoints.size());
	#endif	

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

	if (_imagesChecked)
	{
		printf("In %d images we encountered %d false positives, %d false negatives, and %d multiple detections\n",_imagesChecked,_fp,_fn,_multDetect);
		printf("%f %% of all images was processed correctly in its entirety\n", 100 - (((double)_imagesErr /(double) _imagesChecked)) * 100.);
	}
	if (_signsChecked)
	{
		printf("%d out of %d signs, which is %f %%, was OCRed entirely correctly\n",_OCRcorrect,_signsChecked, (((double)_OCRcorrect /(double) _signsChecked)) * 100.);
		printf ("Average edit distance to correct label: %f\n",_editDist / (double) _signsChecked);
	}
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
