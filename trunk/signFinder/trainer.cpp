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
#include "lib/histogramtool/histogramTool.h"

using namespace std;

int binsize = 64;
//int binsize = 32;

int _curFile=0;

CvHistogram* _posHist;
CvHistogram* _negHist;

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

	// Generate positive / negative histograms.
	CvHistogram* posHist = calculateHistogram(img,mask,binsize,1);
	cvNot(mask,mask);
	CvHistogram* negHist = calculateHistogram(img,mask,binsize,1);

	// Add histograms to whole.
	addHistogram(_posHist,posHist);
	addHistogram(_negHist,negHist);
	
	// Cleanup
	cvReleaseImage(&img);
	cvReleaseImage(&mask);
}
void init()
{
	//_posHist = loadHistogram("posHist.hist");
        //_negHist = loadHistogram("negHist.hist");
	int binsizes[] = {binsize,binsize};
        _posHist = cvCreateHist(2,binsizes, CV_HIST_ARRAY);
        _negHist = cvCreateHist(2,binsizes, CV_HIST_ARRAY);
	cvClearHist(_posHist);
	cvClearHist(_negHist);
	if (!(_posHist && _negHist))
	{
		cerr << "ERROR: posHist.hist and/or negHist.hist histogram failed to load." << endl;
		exit(1);
	}
}

void cleanup()
{
	string filename = "_posHist.hist";
	writeHistogram(_posHist,(char*) filename.c_str());
	filename = "_negHist.hist";
	writeHistogram(_negHist,(char*) filename.c_str());

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
