/* Copyright (c) <2008> <Jaldert Rombouts>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Jaldert Rombouts <rombouts@ai.rug.nl>
 *
 * 2008-05-15.
 *
 * */


#include "histogramTool.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>

void releaseImage(IplImage* img)
{
	cvReleaseImage(&img);
}

void releaseHistogram(CvHistogram* hist_)
{
	cvReleaseHist ( &hist_ );
}

/**
 * Mostly interface for python-wrapper: fills in necessary values for cois and ranges, 
 * converts images and then calls IplImage version of skinDetectBayes
 * 
 * 
 * @param input_ : should be a BGR image in IPL_DEPTH_8U depth and 3 channels!
 * @param color_code_ : determines to which color-space input should be converted
 *
 * @params skinHist_, nonSkinHist_, threshold_, mask_ : see skinDetectBayes IplImage implementation
 */
IplImage* skinDetectBayes(CvMat* input_, CvHistogram* skinHist_, CvHistogram* nonSkinHist_, float threshold_, int color_code_, CvMat* mask_)
{
	//printf("Called by python");
	int cois[3] = {1,1,0};
	float* ranges[3];
   	float r_1[2] = {0, 255};
   	float r_2[2] = {0, 255};
   	float r_3[2] = {0, 255};
	ranges[0] = r_1;
	ranges[1] = r_2;
	ranges[2] = r_3;

	// Put CvMat data into IplImage:	
	IplImage* image_hd = convertCvMatToIpl(input_, IPL_DEPTH_8U, 3);
	IplImage* mask_hd = convertCvMatToIpl(mask_, IPL_DEPTH_8U, 1);

	IplImage* img = NULL;
	// Set cois[], ranges[] and do the conversion to the desired colorspace
	// convert to IPL-header, apply transformation	
	switch (color_code_)
	{
		case 1: // YCrCb
		{
			img = cvCreateImage(cvGetSize(image_hd), IPL_DEPTH_8U, 3);
			cvCvtColor( image_hd, img, CV_BGR2YCrCb );	
			cois[0] = 0;
			cois[2] = 1;
			break;
		}
		case 2: // HSV
		{
			img = cvCreateImage(cvGetSize(image_hd), IPL_DEPTH_8U, 3);
			cvCvtColor( image_hd, img, CV_BGR2HSV );
			r_1[1] = 180; // H runs to 180 
			break;
		}
		case 3: // nRGB
		{
			
			img = cvCreateImage(cvGetSize(image_hd), IPL_DEPTH_32F, 2);
			bgr2normalizedrgb(image_hd, img);
			r_1[1] = 1.0;
			r_2[1] = 1.0;
			break;
		}
		case 4: // CIE-Lab
		{
			
			img = cvCreateImage(cvGetSize(image_hd), IPL_DEPTH_8U, 3);
			cvCvtColor( image_hd, img, CV_BGR2Lab );
			cois[0] = 0;
			cois[2] = 1;
			break;
		}


		default: // Leave image as is:
			printf("SkinDetect: unrecognized color-conversion-code (should be 1-3), passing NULL-pointer..");
	}

	// Now call the IplImage* version of skinDetectBayes:
	IplImage* result = skinDetectBayes(img,  skinHist_, nonSkinHist_, threshold_, cois, ranges, mask_hd);
	
	// Cleanup:
	cvReleaseImageHeader( &image_hd );
	if ( mask_hd )
		cvReleaseImageHeader ( &mask_hd );
	if ( img )
		cvReleaseImage( &img );


	return result;
}



/**
 *  Apply Baysian skin detector on input image, given skin and object color models in given colorspace.
 *	
 *	Note that there is no checking of input type: the user must supply image in correct colorspace, and with appropriate cois and ranges
 *
 * 	@param input : input image in correct colorspace
 * 	@param skinHist : skin histogram model
 * 	@param nonSkinHist : non-skin histogram model
 * 	@param threshold : decision threshold
 *	@param cois : channels of interest, used to determine pixel values to compare with models 
 *	@param ranges : range of values, used to calculate the bin-size (e.g. with 256 possible color-values and a bin-dimension of 32, bin size is 8)
 *	@param mask_ : optional mask
 *
 */
IplImage* skinDetectBayes(IplImage* input, CvHistogram* skinHist, CvHistogram* nonSkinHist, float threshold, int* cois, float** ranges, IplImage* mask_)
{
	///printf("Running skinDetectBayes");
	// Check histogram dimensions:
	int skinHistDims[] = { skinHist->mat.dim[0].size, skinHist->mat.dim[1].size };
	int nonSkinHistDims[] = { nonSkinHist->mat.dim[0].size, nonSkinHist->mat.dim[1].size };
	
	assert(skinHistDims[0] == nonSkinHistDims[0]);
	assert(skinHistDims[1] == nonSkinHistDims[1]);
	
	// Calculate the bin-size:
	float binsizes[2] = {((ranges[0][1] - ranges[0][0]) / (float) skinHistDims[0]), ((ranges[1][1] - ranges[1][0]) / (float) skinHistDims[1])};
	int channels[2];
	// Calculate channel numbers of interest from cois:
	int nset = 0;
	for (int i = 0; i < 3; ++i)
	{
		if (cois[i] == 1)
		{
			channels[nset] = i;
			nset++;
		}
	}
	/*
	for (int i = 0; i < 2; ++i)
		printf("binsizes[%d] = %.3f\n", i, binsizes[i]);

	for (int i = 0; i < 2; ++i)
		printf("Channels[%d] = %d\n", i, channels[i]);
	*/
	IplImage* result = cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
	cvSetZero(result);	
	
	// Storage for temporary values:
	CvScalar pixel;
	float skinHistVal;
	float nonSkinHistVal;

	// Assume that the number of bins is a whole divider.
	// For all pixels, check whether they are in the histogram:
	for (int x = 0; x < input->width; x++)
	{
		for (int y = 0; y < input->height; y++)
		{
			bool checkpixel = true;
			if (mask_ && (cvGet2D(mask_, y, x).val[0] != 255)) // Check whether mask on
				checkpixel = false;


			if (checkpixel)
			{
				// Get pixel value
				pixel = cvGet2D(input, y, x);	

				//printf("Pixel[%d,%d]= (%.3f, %.3f, %.3f )\n", y,x,pixel.val[0], pixel.val[1], pixel.val[2] );
				// NOTE: casting to (int): value 3.9 is bin 3 and 4.0 = > 4.	
				// necessary because we need integer indices for histogram access!
				
				int x_idx = (int)((pixel.val[channels[0]] / binsizes[0]) );
				int y_idx =	(int)((pixel.val[channels[1]] / binsizes[1]) );

				if (x_idx == skinHistDims[0])
					x_idx -= 1;
				if (y_idx == skinHistDims[1])
					y_idx -= 1;


				//printf("idx: %d, %d \n", x_idx, y_idx); 

				skinHistVal = cvQueryHistValue_2D(skinHist, x_idx, y_idx);
				nonSkinHistVal = cvQueryHistValue_2D(nonSkinHist, x_idx, y_idx);

				

				//printf("Shv, nskv = %f, %f\n", skinHistVal, nonSkinHistVal);
				if (( skinHistVal / nonSkinHistVal ) >= threshold)
					cvSetReal2D(result, y, x, (int) 255 /** (skinHistVal / nonSkinHistVal)) */);
			}
		}
	}
	
	return result;
}
/*
 * Deprecated
IplImage* skinDetectBayes(CvMat* input, CvHistogram* skinHist, CvHistogram* nonSkinHist, float threshold)
{
	// Check histogram dimensions:
	
	int skinHistDims[] = { skinHist->mat.dim[0].size, skinHist->mat.dim[1].size };
	int nonSkinHistDims[] = { nonSkinHist->mat.dim[0].size, nonSkinHist->mat.dim[1].size };
	
	assert(skinHistDims[0] == nonSkinHistDims[0]);
	assert(skinHistDims[1] == nonSkinHistDims[1]);
	
	// Calculate the binsize: (assume even histogram bin counts):
	int binsize = (256 / skinHistDims[0]);

	//printf("In skinDetect\n");
    IplImage * image = cvCreateImageHeader(cvSize(input->height, input->width), 8, 3);
    cvGetImage(input, image);

    // Convert image to YCrCb color-space:
    IplImage* h_img = cvCreateImage(cvGetSize(image), 8, 3);
    cvCvtColor( image, h_img, CV_BGR2YCrCb );
	
	//printf("Creating result image\n");
	
	IplImage* result = cvCreateImage(cvGetSize(h_img), IPL_DEPTH_8U, 1);
	cvSetZero(result);	
	
	//printf("Evaluating image pixels\n");
	// Storage for temporary values:
	CvScalar pixel;
	float skinHistVal;
	float nonSkinHistVal;

	// Normalization factors:
	float skinHistCount = cvSum(skinHist->bins).val[0];
	float nonSkinHistCount = cvSum(nonSkinHist->bins).val[0];
	
	// Assume that the number of bins is a whole divider.
	// For all pixels, check whether they are in the histogram:
	for (int x = 0; x < h_img->width; x++)
	{
		for (int y = 0; y < h_img->height; y++)
		{
			// Get pixel value
			pixel = cvGet2D(h_img, y, x);
			skinHistVal = cvQueryHistValue_2D(skinHist, (((int)pixel.val[1]) / binsize) , (((int)pixel.val[2] / binsize))) / skinHistCount; 
			nonSkinHistVal = cvQueryHistValue_2D(nonSkinHist, (((int)pixel.val[1]) / binsize) , (((int)pixel.val[2] / binsize))) / nonSkinHistCount; 
			if ((skinHistVal/nonSkinHistVal) >= threshold)
				//cvSetReal2D(result, y, x, (int) (255 * (skinHistVal / skinHistCount)));
				cvSetReal2D(result, y, x, (int) 255);
		}
	}
	cvReleaseImageHeader(&image);
	cvReleaseImage(&h_img);
	//cvEqualizeHist(result, result);
	return result;
}
*/

/*
 *  Simple LUT skinDetect, note that some errors have to be fixed for this to work again! :)
IplImage* skinDetect(CvMat* yCrCbImg, CvHistogram* hist)
{
	//printf("In skinDetect\n");
    IplImage * image = cvCreateImageHeader(cvSize(yCrCbImg->height, yCrCbImg->width), 8, 3);
    cvGetImage(yCrCbImg, image);

	//cvSaveImage("test_input.png", image);
	//printf("Converting to YCrCb\n");
    // Convert image to YCrCb color-space:
    IplImage* h_img = cvCreateImage(cvGetSize(image), 8, 3);
    cvCvtColor( image, h_img, CV_BGR2YCrCb );
	
	//printf("Creating result image\n");
	
	IplImage* result = cvCreateImage(cvGetSize(h_img), IPL_DEPTH_8U, 1);
	cvSetZero(result);	
	
	//printf("Evaluating image pixels\n");
	CvScalar pixel;
	float histVal;

	float histCount = cvSum(hist->bins).val[0];
	//printf("HistCount %f", histCount);
	// Assume that the number of bins is a whole divider.
	// For all pixels, check whether they are in the histogram:
	for (int x = 0; x < h_img->width; x++)
	{
		for (int y = 0; y < h_img->height; y++)
		{
			// Get pixel value
			pixel = cvGet2D(h_img, y, x);
			//printf("Calculating histval for %d, %d, pixel values: %f, %f \n", x, y, pixel.val[1], pixel.val[2]);	
			//printf("Lookup: %d, %d\n", ((int)pixel.val[1] / BINSIZE) , ((int)pixel.val[2] / BINSIZE));
			histVal = cvQueryHistValue_2D(hist, (((int)pixel.val[1]) / BINSIZE) , (((int)pixel.val[2] / BINSIZE))); 
			//printf("Checking bound for: %f\n", histVal);	
			if (histVal > THRESHOLD)
				//if ((histVal / histCount) >= 0.25)
				cvSetReal2D(result, y, x, (int) (255 * (histVal / histCount)));
			
			//printf("Original: %f, Add: %f, Result: %f\n", old, add, cvQueryHistValue_2D(a,i,j) );
		}
	}
	cvEqualizeHist(result, result);
	return result;
}
*/

IplImage* backProject(CvMat* yCrCbImg, CvHistogram* hist)
{
//IplImage* img = cvCreateImage(cvGetSize(yCrCbImg), IPL_DEPTH_8U, 3);
//	cvGetImage(yCrCbImg, img);
	IplImage * image = cvCreateImageHeader(cvSize(yCrCbImg->height, yCrCbImg->width), 8, 3);
    cvGetImage(yCrCbImg, image);

	// Convert image to HSV color-space:
	IplImage* h_img = cvCreateImage(cvGetSize(image), 8, 3);
    cvCvtColor( image, h_img, CV_BGR2YCrCb );

	// Creating images to pass by reference;
    IplImage* y_plane  = cvCreateImage( cvGetSize(image), 8, 1);
    IplImage* cr_plane = cvCreateImage( cvGetSize(image), 8, 1);
    IplImage* cb_plane = cvCreateImage( cvGetSize(image), 8, 1);
	 
	// We are only interested in the Hue and Saturation values:
    IplImage* planes[] = {cr_plane, cb_plane};    
        
    cvCvtPixToPlane(h_img, y_plane, cr_plane, cb_plane, 0);

	IplImage* result = cvCreateImage(cvGetSize(yCrCbImg), IPL_DEPTH_8U, 1);
	cvCalcBackProject(planes, result, hist);
	//cvCalcBackProjectPatch(input, result, cvSize(50,50), hist, CV_COMP_BHATTACHARYYA,1);
	return result;
}

/* Function for adding histograms.*/
void addHistogram(CvHistogram* a, CvHistogram* b)
{
	int aHistDims[] = { a->mat.dim[0].size, a->mat.dim[1].size };
	int bHistDims[] = { b->mat.dim[0].size, b->mat.dim[1].size };

	assert(aHistDims[0] == bHistDims[0]);
	assert(aHistDims[1]	== bHistDims[1]);
	
	//printf("Adding to histogram\n");
	int dims[] = { a->mat.dim[0].size, a->mat.dim[1].size };
	CvMat* resultBins = (CvMat*)a->bins;

	float add = 0;
	float old = 0;
	float nw = 0;
	for (int i = 0; i < dims[0]; i++)
	{
		for (int j = 0; j < dims[1]; j++)
		{
			old = cvQueryHistValue_2D(a, i, j);
			add = cvQueryHistValue_2D(b, i, j); 
			nw = old + add;
			cvSetReal2D(resultBins, i, j, nw);
			//assert(cvQueryHistValue_2D(a, i, j) == (nw) );
			//printf("Original: %f, Add: %f, Result: %f\n", old, add, cvQueryHistValue_2D(a,i,j) );
		}
	}
}


void writeHistogram(CvHistogram * hist, char* filename)
{
		cvSave(filename, hist->bins, "hist", "Histogram saved by HistTool::writeHistogram.");
}

CvHistogram* loadHistogram(char* filename)
{
	CvMatND* bins = (CvMatND*) cvLoad(filename);

	int dims[] = {bins->dim[0].size, bins->dim[1].size};
	
	CvHistogram* hist = cvCreateHist(2, dims, CV_HIST_ARRAY);
	// Read histogram bin values:
	for (int i = 0; i < dims[0]; i++)
	{
		for (int j = 0; j < dims[1]; j++)
		{
			*((float*)cvPtr2D( (hist)->bins, i, j, 0 ))	= (float) cvGet2D(bins, i, j).val[0];
		}
	}
	return hist;
}

/* Calculates Cb and Cr histogram for input image
 * The mask specifies for which part of the image the 
 * histogram should be calculated. Note that the mask must be
 * a gray image!
 * It returns a pointer to a CvHistogram, which can
 * then be used for comparison to other histograms.
 * */
CvHistogram* histogram(CvMat* img, CvMat* mask, char* filename, int dim)
{
	// Make sure that mask is a GREY (CvMat*) image!
	// Convert CvMat* to IplImage*:
	IplImage * image = cvCreateImageHeader(cvSize(img->height, img->width), 8, 3);
	cvGetImage(img, image);

	// Histograms only work one 1-channel images, so, 
	// we need to split the HSV-image in three planes
	// Based on: (search histogram)
	// http://www.cs.indiana.edu/cgi-pub/oleykin/website/OpenCVHelp/
	
	//printf("Converting to YCrCb-planes.\n");	
	
	// Convert image to HSV color-space:
	IplImage* h_img = cvCreateImage(cvGetSize(image), 8, 3);
    cvCvtColor( image, h_img, CV_BGR2YCrCb );

	// Creating images to pass by reference;
    IplImage* y_plane  = cvCreateImage( cvGetSize(image), 8, 1);
    IplImage* cr_plane = cvCreateImage( cvGetSize(image), 8, 1);
    IplImage* cb_plane = cvCreateImage( cvGetSize(image), 8, 1);
	 
	// We are only interested in the Cr and Cb values:
    IplImage* planes[] = {cr_plane, cb_plane};    
        
    cvCvtPixToPlane(h_img, y_plane, cr_plane, cb_plane, 0);

	// Initialize histogram struct values:
	
	int dims[] = {dim, dim};

	//printf("Creating empty histogram.\n");
	CvHistogram* hist = cvCreateHist(2, dims, CV_HIST_ARRAY);
		
	//printf("Calculating histogram.\n");

	// Calculate the actual histogram:
	cvCalcHist(planes, hist, 0, mask);

	//printf("Writing gnuplot file\n");
	// Writing gnuplot file for histogram	
	//printGNUplot(hist, filename);

	// Clean up:
	for(int i = 0; i < 2; i++)
	{
		cvReleaseImage(&planes[i]);
	}
	cvReleaseImage(&y_plane);
	cvReleaseImage(planes);
	cvReleaseImage(&h_img);
	// This release does not work, only release header
	cvReleaseImageHeader(&image);

	//printf("Returning.\n");
	//writeHistogram(hist);
	return hist;
}

/*Write a data-file that is readable for GNUPLOT to a file*/
void printGNUplot(CvHistogram * hist, char* filename)
{
	int dims[] = { hist->mat.dim[0].size, hist->mat.dim[1].size };
	
	FILE* fp = fopen(filename, "w");
	float count = 0;

	// Read histogram bin values:
	for (int i = 0; i < dims[0]; i++)
	{
		for (int j = 0; j < dims[1]; j++)
		{
			fprintf(fp,"%i\t%i\t%.0f\n", i, j, cvQueryHistValue_2D(hist, i, j)); 
			count +=  cvQueryHistValue_2D(hist, i, j);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	printf("printGNUplot::Count in histogram: %f\n", count);
}




// Rn = R / (R+G+B) and Gn = G / (R+G+B), all values will be in range [0..1]
void bgr2normalizedrgb(IplImage* src_, IplImage* dst_)
{
	float rgbSum = 0;
	
	for ( int y = 0; y < src_->height; y++ )
	{
		uchar* ptr_src_8U_C3 = ( uchar* )( src_->imageData +
				y * src_->widthStep );
		float* ptr_dst_32F_C2 = ( float* )( dst_->imageData +
				y * dst_->widthStep );		

		
		for ( int x = 0; x < src_->width; x++ )
		{
			rgbSum = 0;
			
			rgbSum += (float)ptr_src_8U_C3[3 * x + 0];
			rgbSum += (float)ptr_src_8U_C3[3 * x + 1];
			rgbSum += (float)ptr_src_8U_C3[3 * x + 2];
			
			if (rgbSum != 0)
			{
				ptr_dst_32F_C2[2 * x + 0] = ptr_src_8U_C3[3 * x + 2] / rgbSum; // Rn
				ptr_dst_32F_C2[2 * x + 1] = ptr_src_8U_C3[3 * x + 1] / rgbSum; // Gn
			}
			else
			{
				ptr_dst_32F_C2[2 * x + 0] = 0;
				ptr_dst_32F_C2[2 * x + 1] = 0;
			}
		}
	}
}

CvHistogram* calculateNegHistogram(CvMat* in_, CvMat* mask, int dim, int code)
{
	// Tijs hack
	// we invert the mask in-place.
	cvNot(mask,mask);
	return calculateHistogram(in_, mask, dim, code);

}

// Tijs
CvHistogram* calculateHistogram(IplImage* in_, IplImage* mask_, int dim, int code)
{
	CvMat in = cvMat(in_->height,in_->width,CV_8UC3);
	CvMat mask = cvMat(in_->height,in_->width,CV_8UC1);
	in = *(cvGetMat(in_, &in));
	mask = *(cvGetMat(mask_,&mask));
	return calculateHistogram(&in, &mask,dim,code);

}

CvHistogram* calculateHistogram(CvMat* in_, CvMat* mask, int dim, int code)
{

		

	int cois[3];
	float* ranges[3];
   	float r_1[2] = {0, 255};
   	float r_2[2] = {0, 255};
   	float r_3[2] = {0, 255};
	ranges[0] = r_1;
	ranges[1] = r_2;
	ranges[2] = r_3;

	switch ( code )
	{
		case 1: // YCrCb
		{
			cois[0] = 0;
			cois[1] = 1;
			cois[2] = 1;
			return calculateHistogram(in_, mask, cois, dim, CV_BGR2YCrCb, ranges);
			break;	
		}
		case 2: // HSV
		{
			cois[0] = 1;
			cois[1] = 1;
			cois[2] = 0;
			r_1[1] = 180;

			return calculateHistogram(in_, mask, cois, dim, CV_BGR2HSV, ranges);
			break;	
		}
		case 3: // nRGB
		{
			cois[0] = 1;
			cois[1] = 1;
			cois[2] = 0;
			r_1[1] = 1.0;
			r_2[1] = 1.0;
			r_3[1] = 0;
			
			IplImage* image = convertCvMatToIpl(in_, IPL_DEPTH_8U, 3);
			IplImage* msk = convertCvMatToIpl(mask, IPL_DEPTH_8U, 1);

			IplImage* nrgb = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 2);
			bgr2normalizedrgb(image, nrgb);

			CvHistogram* result = calculateHistogram(nrgb, msk, cois, dim, -1, ranges);
			
			cvReleaseImage( &nrgb );	
			cvReleaseImageHeader( &image );
			cvReleaseImageHeader( &msk );
			
			return result;	
			break;	
		}
		case 4: // CIE-Lab
		{
			
			cois[0] = 0;
			cois[1] = 1;
			cois[2] = 1;
			return calculateHistogram(in_, mask, cois, dim, CV_BGR2Lab, ranges);

			
			break;
		}

		default:
			printf("Unknown code!");
	}

	return NULL;
}


IplImage* convertCvMatToIpl(CvMat* in_, int depth_, int channels_)
{
	IplImage* image = NULL;
	if (in_)
	{
		// Get CvMat data, and create a IplImage header for it:
		image = cvCreateImageHeader(cvSize(in_->height, in_->width), depth_, channels_);
    	cvGetImage(in_, image);
	}
	return image;
}

CvHistogram* calculateHistogram(CvMat* in_, CvMat* mask_, int* cois_, int dim_, int code_, float** ranges)
{
	// Get CvMat data, and create a IplImage header for it:
	IplImage * image = cvCreateImageHeader(cvSize(in_->height, in_->width), 8, 3);
    cvGetImage(in_, image);

	IplImage* mask = NULL;
	if (mask_)
	{	
		mask = cvCreateImageHeader(cvSize(mask_->height, mask_->width), 8, 1);
    	cvGetImage(mask_, mask);
	}

	// Now call the IplImage* version of histogram:
	CvHistogram* result = calculateHistogram(image, mask, cois_, dim_, code_, ranges);
	
	// Cleanup (note that image and mask only contain a pointer to the image-data:
	cvReleaseImageHeader( &image );
	if (mask)
		cvReleaseImageHeader( &mask );

	return result;
}

CvHistogram* calculateHistogram(IplImage* in_, IplImage* mask_, int* cois_, int dim_, int code_, float** ranges)
{
	
	// Histograms only work one 1-channel images, so, 
	// we need to split the image in nChannel planes
	// Based on: (search histogram)
	// http://www.cs.indiana.edu/cgi-pub/oleykin/website/OpenCVHelp/

	// Will contain pointer to copy of in_, 
	// with optional color-space transform
	IplImage* input;
	// Convert image to desired color-space
	// \note user is responsible for providing correct code!
	if (code_ != -1)
	{
		///\todo nChannels for target colorspace could be different from target image.
		input = cvCreateImage(cvGetSize(in_), in_->depth, in_->nChannels);
		cvCvtColor( in_, input, code_ );
	}
	else
		input = in_;//cvCloneImage(in_);


	// Array of images that will contain the image-planes 
	// for which histograms will be calculated:
	IplImage* planes[] = {NULL, NULL, NULL, NULL};

	// Will contain pointers to channels of which histogram should be calculated;
	vector<IplImage*> targets;
	targets.reserve(4);
	
	// Create images for splitting source-image:	
	for (int i = 0; i < input->nChannels; ++i)
	{
		planes[i] = cvCreateImage( cvGetSize(input), input->depth, 1 );
		// Check whether user wants histogram of this channel:
		if (cois_ && cois_[i] == 1) // if channels of interest set
		{
			targets.push_back(planes[i]);
		}
		else if (!cois_)// no coi set: add all planes:
			targets.push_back(planes[i]);
	}

	if (input->nChannels != 1)
	{
		// Split input into planes, note that non-existing planes are automatically NULL!	
		cvCvtPixToPlane( input, planes[0], planes[1], planes[2], planes[3] );
	}
	else
		cvCopy(input, planes[0]);
	
	// Convert `target' to IplImage* [], since this is needed for CvcalcHist:
	IplImage* target[targets.size()];
	for (uint i = 0; i < targets.size(); ++i)
		target[i] = targets[i];	

	///\todo: check whether this can be done better :)
	// Initialize histogram struct values:
	int dims[targets.size()];
	for (uint i = 0; i < targets.size(); ++i)
	{
		dims[i] = dim_;
	}
	
	CvHistogram* hist = cvCreateHist( targets.size(), dims, CV_HIST_ARRAY, ranges);

	
 	//printf("calchist::dims=%d, bins=%d; ranges[0] = (%f,%f)", targets.size(), dims[0], ranges[0][0], ranges[0][1] );

#ifdef DEBUG		
	printf("Calculating histogram.\n");
#endif
	// Calculate the actual histogram:
	cvCalcHist(target, hist, 0, mask_);

	// Clean up:
	///\todo Check cleanup!
	for(int i = 0; i < in_->nChannels; i++)
		cvReleaseImage( &planes[i] );

	if (code_ != -1)
		cvReleaseImage(&input);

	return hist;
}








