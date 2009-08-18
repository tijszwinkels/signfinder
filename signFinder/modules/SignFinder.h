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
 * The Original Code is SignFinder.h .
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

#include <string>
#include "lib/histogramtool/histogramTool.h"
#include "lib/bloblib/Blob.h"
#include "lib/bloblib/BlobResult.h"
#include "OpenSURF/surflib.h"

using namespace std;

class SignFinder
{
	public:
		struct DetectPerformance
		{
			int _fp, _fn, _multDetect, _imagesChecked, _imagesErr;
			DetectPerformance()
			{
				_fp=0, _fn=0, _multDetect=0, _imagesChecked=0, _imagesErr=0;
			}
		};
		struct OcrPerformance
		{
			int _OCRcorrect,_signsChecked; 
			double _editDist;
			OcrPerformance()
			{
				_OCRcorrect=0,_signsChecked=0;
				_editDist = 0;
			}
		};

	private:
		bool _debug, _showPerformance;
		double _histThreshold;
		CvHistogram* _posHist;
		CvHistogram* _negHist;
		IpVec _surfpoints;
		DetectPerformance _detperf;
		OcrPerformance _ocrperf;
		int XRES, YRES;

	/* public interface */
	public:
		SignFinder();
		~SignFinder();

		string readSigns(char* file, IplImage* result = NULL);
		void performanceMeasurements();

	/* getters and setters*/
		void setThreshold(double thr) {_histThreshold = thr;}
		void setRes(int x, int y) {XRES = x; YRES=y;}
		void disableResize() {setRes(0,0);}
		void setDebug(bool dbg=true) {_debug = dbg;}
		void setShowPerformance(bool show=true) {_showPerformance = show;}

	/* support functions*/
	protected:
		void init();
		void cleanup();
		void loadHistograms();
		void loadSurf();
		IplImage* resize(IplImage* img);
		IplImage* histMatch(IplImage* img, IplImage* vis=NULL);
		void processSurf(IplImage* img);
		CBlobResult classifyBlobs(CBlobResult& blobs, char* file, CvSize size, IplImage* vis=NULL);
		string processBlob(CBlob* currentBlob, char* file, IplImage* result, int& prevY, IplImage* histMatchVis);
		void drawConvexHull(CBlob* blob, IplImage* img, int i);
		

};

void signFinderCleanup();
void signFinderInit();
void processFile(char* file);
