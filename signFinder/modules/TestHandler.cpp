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
#include <math.h>
#include <iostream>
#include <fstream>
#include "TestHandler.h"

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

	cout << estimation->width << " " << label->width << endl;

	// Resize label to size that the classifier is using.
	if (( estimation->width != label->width ) || ( estimation->height != label->height ))
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
	// We are also correcting for when the label has a different size than the image.
	double labelSurface = cvSum(label).val[0] / 255.;
	double sizefact = (label->width*label->height) /  (blob->width * blob->height); // factor that the label is bigger	
	labelSurface /= sizefact;

	fp = fp * (blob->width * blob->height) / labelSurface;

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
	cvSet(img,cvScalar(0,0,0,0));
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
bool checkLabeledBlobs(CBlobResult& detectedBlobs, IplImage* origImg, char* file, int& fp, int& fn, int& multipleDetections, CBlobResult* correctBlobsOut, CBlobResult* incorrectBlobsOut)
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
        IplImage* detectedMask = cvCreateImage(cvGetSize(origImg), IPL_DEPTH_8U, 3);

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

/**
 * Compares the OCRed text from a streetsign with known-correct labels if present
 */
int compareText(string detected, char* imgfile)
{
	string label;	

	// read labeled text from file;
	string textfile(imgfile);
        ifstream ifs((textfile+=".txt").c_str());

	// For multiple signs, assume that the reading with the lowerst edit-distance is the actual correct one.
	int mindist = 1000;	
	while ((!ifs.eof()) && (!ifs.fail()))
	{
		getline(ifs,label);
		//cout << label << endl;
		if (label == "")
			break;

		// trim leading and trailing whitespace characters.
		label = trim(label);
		detected = trim(detected);

		// return the levenshtein distance between the two strings.
		printf("Comparing \"%s\" and \"%s\".\n",detected.c_str(),label.c_str());
		int dist = levenshtein(label.c_str(),detected.c_str());
		if (dist < mindist)
			mindist = dist;
	}

	ifs.close();
	return mindist; // 1000 if error occured.

	
}

/* Support Functions */

/* Computes the edit-distance or Levenshtein distance between two strings */

int min(int a, int b, int c)
{
    if (a < b)
        if (b<c)
            return a;
        else
            return c;
    else if (b < c)
        return b;
    else
        return c;
}

int levenshtein(const char* a, const char* b)
{
    int al = strlen(a);
    int bl = strlen(b);
    int matrix[al+1][bl+1];

    for (int i=0; i<al+1; i++)
        for (int j=0; j<bl+1; j++)
        {
            matrix[i][j] = 0;
            matrix[i][0] = i;
                    matrix[0][j] = j;
        }


    for (int i=1; i<al+1; i++)
        for (int j=1; j<bl+1; j++)
        {
            int cost;
            if (a[i]==b[j])
                cost=0;
            else
                cost=1;     
        
            matrix[i][j] = min( matrix[i-1][j]+1,
                        matrix[i][j-1]+1,
                        matrix[i-1][j-1] + cost );
        }

    return matrix[al][bl];      
}

string trim(string in)
{
	if (in == "")
		return in;

	size_t startpos = in.find_first_not_of(" \t");  
	size_t endpos = in.find_last_not_of(" \t");
	return in.substr(startpos, endpos-startpos + 1);
}
