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
 * The Original Code is OCRWrapper.cpp .
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

#include "OCRWrapper.h"
#include "TestHandler.h"
#include <opencv/highgui.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>

const bool _debug = false;

bool isCap(char c)
{
	return ((c >= 65) && (c <= 90));
}

bool isAllCaps(string& in, double frac = 0.5, int n=5)
{
	int countCaps = 0;
	for (int i=0; i < ((n > in.length()) ? in.length() : n); ++i)
		if (isCap(in[i]))
			++countCaps;	

	if ( ((double) countCaps / (double) in.length()) > frac)
		return true;
	return false;
}

void removeOneCharWords(string& in)
{
	int charcount=0, i;
	for (i=0; i<in.length(); ++i)
	{
		if (in[i] != ' ')
			++charcount;
		else
		{ 
			if (charcount < 2)
			{
				if (i!=0)
				{
					in.erase(i-1,1);
					--i;
				}
			}
			charcount = 0;
		}
	}
	if ((charcount < 2) && (i!=0))
		in.erase(i-1,1);
}

void detectWrongCapitalI(string& in)
{
	if (isAllCaps(in))
		return;

	int charcount=0, i;
	for (i=0; i<in.length(); ++i)
	{
		if (in[i] != ' ')
			++charcount;
		else
			charcount = 0;
	
		// If we're not at the beginning of the word,
		// and we encounter a capital I, it's probably an l.
		if ((charcount > 1) && (in[i] == 'I'))
			in[i] = 'l';
	}
}

/* This function looks at the beginning of the street sign.
 * If the characters before the second captical letter are similar to the words 'de' or 'het',
 * the characters are replaced by one of these words.*/
void correctLidwoorden(string& in)
{
	if (isAllCaps(in))
		return;

	// Find begin of second word.
	int i;
	for (i=3; i<in.length(); ++i)
		if (isCap(in[i]))
				break;
	int secondCapPos = i;

	// Sanity checks
	if (secondCapPos == in.length())
		return;
	else if (secondCapPos > 6) // Allow two spurious characters at most.
		return;
		
	// Found whether the first word is more similar to 'De' or 'Het'.
	string foundWord = in.substr(0,secondCapPos);
	int numCandidates = 2;
	string candidates[] = {"De ", "Het "};

	int minDist = 10;
	int index = -1; 
	for (i=0; i< numCandidates; ++i)
	{
		int dist = levenshtein(candidates[i].c_str(), foundWord.c_str());	
		if (dist < minDist)
		{
			minDist = dist;
			index = i;
		}
	}	
	
	if (_debug) fprintf(stderr,"Word %s is most similar to %s with a levenshtein distance of %d\n",foundWord.c_str(),candidates[index].c_str(),minDist);

	if (minDist <=2)
		in.replace(0,secondCapPos,candidates[index]);		
}

string extractText(IplImage* sign, CvHistogram* _posHist, CvHistogram* _negHist)
{
	// Convert image to greyscale.
     	IplImage* grey = cvCreateImage(cvGetSize(sign), IPL_DEPTH_8U, 1);
        //cvCvtColor(sign,grey,CV_RGB2GRAY);
        // To increase the contrast of blue signs, we're greyscaling the red channel.
	cvSetImageCOI(sign,3);
	cvCopy(sign,grey);
	cvSetImageCOI(sign,0);

	// save the image for tesseract + cleanup
	//cvSaveImage("OCRsign.tif",histMatched);
	cvSaveImage("OCRsign.tif",grey);
	cvReleaseImage(&grey);

	// Crunch the image through tesseract, and gather the results.
	if (_debug)
		system("tesseract OCRsign.tif OCR nobatch modules/signOCR.conf");
	else
		system("tesseract OCRsign.tif OCR nobatch modules/signOCR.conf 2> /dev/null");
	string result;
	result[0] = 0;
	ifstream ifs("OCR.txt");
	if (!ifs.bad())
		getline(ifs, result);
	else
		cerr << "WARNING: Could not read OCR results. Is tesseract properly installed?" << endl;
	
	// Cleanup
	remove("OCRsign.tif");
	remove("OCR.txt");
	remove("OCR.raw");
	remove("OCR.map");

	if (_debug) cerr << "** before OCR heuristics:\t" << result << endl;	
	removeOneCharWords(result);
	detectWrongCapitalI(result);
	correctLidwoorden(result);
	if (_debug) cerr << "** after OCR heuristics:\t" << result << endl;	
	return result;
}
