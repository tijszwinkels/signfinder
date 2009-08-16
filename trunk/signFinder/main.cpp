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
 * The Original Code is main.cpp for the signFinder.
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
#include "modules/SignFinder.h"

const int WINDOWX = 1024;
const int WINDOWY = 768;

int _curFile = 0;

SignFinder sf;

void processFile(char* file)
{
		IplImage* vis = cvCreateImage(cvSize(1600,1200), IPL_DEPTH_8U,3);
                string result = sf.readSigns(file,vis);
                cout << result ;
                string resultfile(file);
                cvSaveImage((resultfile+"_result.jpg").c_str(),vis);

		cvShowImage("signFinder",vis);
                cvReleaseImage(&vis);
}

int main(int argc, char** argv)
{
        if (argc < 2)
        {
                cerr << "Usage: " << argv[0] << " <image-files>" << endl;
                exit(0);
        }

	cvNamedWindow("signFinder",0);
        cvResizeWindow("signFinder", WINDOWX, WINDOWY);	

        // iterate through all files.
        while (++_curFile < argc)
	{
		processFile(argv[_curFile]);
		cvWaitKey(20);
	}

        return 0;
}

