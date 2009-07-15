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
 * The Original Code is CornerFinder.cpp .
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
#include <math.h>
#include "CornerFinder.h"


using namespace std;


double pointDist(CvPoint& p0, CvPoint& p1)
{
	return sqrt(pow(p0.x-p1.x,2)+pow(p0.y-p1.y,2));	
}

void findCorners(CBlob& blob, CvPoint* corners, int numCorners, double distThr, double angleThr )
{
	int cornIndex = 0;

	// Initialize the corner-array.
	memset(corners,0,sizeof(CvPoint) * numCorners);

        CvSeq* hull;
        blob.GetConvexHull(&hull);
        CvPoint pt0 = **CV_GET_SEQ_ELEM( CvPoint*, hull, hull->total - 1 );
	CvPoint prevCorner;
	double prevDirection = -99;
        for (int j=0; j< hull->total; ++j)
        {
                CvPoint pt1 = **CV_GET_SEQ_ELEM( CvPoint*, hull, j );
		
		// Compare current line-direction with previous line-direction.
		double direction = atan((double)(pt0.x - pt1.x)/(double) (pt0.y - pt1.y));
		if (prevDirection != -99)
		{
			double deltaDirection = fabs(direction - prevDirection);
			//cout << "deltaDirection: " << deltaDirection << "/" << angleThr << " pointDist: " << pointDist(pt0,prevCorner) << "/"  << distThr  << endl;
			if ( (deltaDirection > angleThr) && (pointDist(pt0,prevCorner) > distThr) )
			{
				if (cornIndex > (numCorners -1))
				{
					cerr << "findCorners:: found too many corners" << endl;
					return;
				}
				// found a corner, add it to the list.
				printf("Found corner %d at (%d,%d).\n",cornIndex,pt0.x,pt0.y);
				corners[cornIndex++] = pt0;
				prevCorner = pt0;
			}
				
		}
                
		// Set-up next iteration.
		pt0 = pt1;
		prevDirection = direction;
        }
}	

/* FIXME:
 * This corner-finder algorithm is simple, and 'just works'. However, it is dependent on parameter tweaking, and is therefore not likely
 * too work in generic circumstances, no to mention for finding arbitrary numbers of corners in other geometrical figures.
 *
 * A nice method for finding corners in convex approximately even-sided geometrical figures, would be to use weighed k-means clustering.
 * Every point could be an element to cluster, with the angle of the convex hull at that point being the weight.
 * Additional intelligence would be needed to retain the order of the corners, though.
 */
