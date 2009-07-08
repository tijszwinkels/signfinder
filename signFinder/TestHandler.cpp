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
 * The Original Code is TestHandler.cpp .
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


#include <stdio.h>
#include <iostream>
#include "TestHandler.h"

using namespace std;

/** Given a classifier-estimated mask and a known-correct labeled mask,
 *  this function computes the true positive and false positive fraction relative to the label:
 *  What part of the label is recognized, and how much is recognizes that is not part of the label, relative to the surface of the label. 
 *
 *  @param _fp surface of false-positives 
 *  @return The fraction of the known-correct label that was marked a positive.
 */
double compareMasks(IplImage* estimation, IplImage* label, double* _fp)
{
	IplImage* compLabel = NULL;
	IplImage* intersection = cvCreateImage(cvGetSize(estimation), IPL_DEPTH_8U, 3);

	// Resize label to size that the classifier is using.
	if ((cvGetSize(estimation).width != cvGetSize(label).width) || (cvGetSize(estimation).height != cvGetSize(label).height))
	{
		compLabel = cvCreateImage(cvGetSize(estimation), IPL_DEPTH_8U, 3);
		cvResize(label,compLabel);
		label = compLabel;
	}

	// Calculate statistics
	cvAnd(estimation,label,intersection);	
	double labelSurface = cvSum(label).val[0] / 255.;
	double estimationSurface = cvSum(estimation).val[0] / 255. ;
	double tpSurface = cvSum(intersection).val[0] / 255.;
	double tp = tpSurface / labelSurface; 
	double fp = (estimationSurface - tpSurface) / (label->width * label->height);

	printf ("labelSurface: %f, estimationSurface: %f, tpSurface: %f, tp: %f, fp: %f\n",labelSurface, estimationSurface, tpSurface, tp, fp);

	// Cleanup
	if (compLabel)
		cvReleaseImage(&compLabel);
	cvReleaseImage(&intersection);

	// Return measurements 
	if (_fp)
		*_fp = fp;
	return tp;
}

bool blobCorrect(IplImage* blob, IplImage* label, double numBlobs = 1)
{
	double fp;
	double tp = compareMasks(blob,label,&fp);

	// correct the false-positive for the surface of the estimated surface.
	// or: 'How much more than the area that should be detected is detected'.
	double labelSurface = cvSum(label).val[0] / 255.;
	fp = fp * (label->width * label->height) / labelSurface;

	if ((tp * numBlobs) > 0.60) // At least 90% of the label is matched.
		if (fp < (0.25 / numBlobs)) // the 'false positive' area is at most 25% bigger than the label.
			return true;

	return false;
}


/**
 * Fills an image with a convex hull as described by a CvSeq
 */
void fillConvexHull(IplImage* img, CvSeq* hull, CvScalar color)
{
	CvPoint points[hull->total];
	for (int j=0; j< hull->total; ++j)
		points[j] = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
	cvFillConvexPoly(img,points,hull->total,color);
}

/**
 * Fills an image with a convex hull around the pixels in a blob.
 */
 // FIXME: Memory mangement for CvSeq.
void fillConvexHull(IplImage* img, CBlob* blob, CvScalar color)
{
	CvSeq* hull;
        blob->GetConvexHull(&hull);
	fillConvexHull(img,hull,color);
}


/* This function judges whether detected blobs corresponds with one of the known-correct labeled area's */
// FIXME: Things go wrong if detection image has a different size than the mask.
bool checkLabeledBlobs(CBlobResult& detectedBlobs, char* file, int& fp, int& fn, int& multipleDetections, CBlobResult* correctBlobsOut, CBlobResult* incorrectBlobsOut)
{
	// See if we can find a mask for this file.
        string maskfile(file);
        IplImage* labeledMask = cvLoadImage((maskfile+"_mask.png").c_str());
	if (!labeledMask)
	{
		cerr << "Warning:: Couldn't open mask " << maskfile << endl;
		return false;
	}

	// Detect the blobs in the mask.
        IplImage* labeledMaskbw = cvCreateImage(cvGetSize(labeledMask), IPL_DEPTH_8U, 1);
	cvCvtColor(labeledMask, labeledMaskbw, CV_RGB2GRAY);
	CBlobResult maskblobs = CBlobResult( labeledMaskbw, NULL, 0, false );

	// filter blobs 
	maskblobs.Filter( maskblobs , B_EXCLUDE, CBlobGetArea(), B_LESS, (labeledMask->height*labeledMask->width) / 1000 ); 
        // Blobs not in contact with sides of image.
        maskblobs.Filter( maskblobs, B_EXCLUDE, CBlobGetMinX(), B_EQUAL, 0);
        maskblobs.Filter( maskblobs, B_EXCLUDE, CBlobGetMaxX(), B_EQUAL, labeledMask->width-1);
        maskblobs.Filter( maskblobs, B_EXCLUDE, CBlobGetMinY(), B_EQUAL, 0);
        maskblobs.Filter( maskblobs, B_EXCLUDE, CBlobGetMaxY(), B_EQUAL, labeledMask->height-1);

	int correctMaskBlobs[maskblobs.GetNumBlobs()];
	for (int i=0; i<maskblobs.GetNumBlobs(); ++i)
		correctMaskBlobs[i]=0;
        IplImage* detectedMask = cvCreateImage(cvGetSize(labeledMask), IPL_DEPTH_8U, 3);

	// Iterate through each of the blobs in each of the mask, to see if they correspond.
	for (int detectedBlobI = 0; detectedBlobI < detectedBlobs.GetNumBlobs(); ++detectedBlobI)
	{
		bool blobFound = false;
		for (int maskBlobI = 0; maskBlobI < maskblobs.GetNumBlobs(); ++maskBlobI)
		{
			// fill a mask for the detected blob by filling the convex hull.
			cvSet(detectedMask,cvScalar(0,0,0));
			fillConvexHull(detectedMask,detectedBlobs.GetBlob(detectedBlobI),CV_RGB(255,255,255));

			// fill a mask for the labeled blob by filling the convex hull, we're re-using the labeledMask.
			cvSet(labeledMask,cvScalar(0,0,0));
			fillConvexHull(labeledMask,maskblobs.GetBlob(maskBlobI),CV_RGB(255,255,255));
						
			// check for correspondence.
			if (blobCorrect(detectedMask, labeledMask))
			{
				blobFound = true;
				++correctMaskBlobs[maskBlobI];
				if (correctBlobsOut)
					correctBlobsOut->AddBlob(detectedBlobs.GetBlob(detectedBlobI));
			}
		}
		
		if ((!blobFound) && incorrectBlobsOut)
			incorrectBlobsOut->AddBlob(detectedBlobs.GetBlob(detectedBlobI));
	}

	int correctMaskBlobsCnt = 0;
	int correctDetectedBlobsSum = 0;
	// Calculate false-positives and false-negatives.
	for (int i=0; i<maskblobs.GetNumBlobs(); ++i)
	{
		correctDetectedBlobsSum += correctMaskBlobs[i];
		if (correctMaskBlobs[i])
			++correctMaskBlobsCnt;
	}
	
	multipleDetections = correctDetectedBlobsSum - correctMaskBlobsCnt;	
	fp = detectedBlobs.GetNumBlobs() - correctDetectedBlobsSum;
	fn = maskblobs.GetNumBlobs() - correctMaskBlobsCnt;	

	// Cleanup
	cvReleaseImage(&detectedMask);
	cvReleaseImage(&labeledMaskbw);
	return true;
}

