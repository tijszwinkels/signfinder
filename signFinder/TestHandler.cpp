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
 * The Original Code is maskMaker.
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
#include "TestHandler.h"

/** Given a classifier-estimated mask and a known-correct labeled mask,
 *  this function computes the true positive and false positive fraction relative to the label:
 *  What part of the label is recognized, and how much is recognizes that is not part of the label, relative to the surface of the label. 
 *
 *  @param _fp surface of false-positives / surface of known-correct label
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
	double fp = (estimationSurface - tpSurface) / labelSurface;

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

bool blobCorrect(IplImage* blob, IplImage* label,double numBlobs)
{
	double fp;
	double tp = compareMasks(blob,label,&fp);

	if ((tp * numBlobs) > 0.9) // At least 90% of the label is matched.
		if (fp < (0.5 / numBlobs)) // the 'false positive' area is at most 50% bigger than the label.
			return true;

	return false;
}

