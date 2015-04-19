
```
Usage:
     signFinder <options> [image-file.jpg ...]

Description:
     This program tries to find and read dutch street signs in image files.
     By default, Original images with marked street-signs, cuts of the street signs,
     and their names are written to <filename>_result.jpg. These results are displayed in a window while
     classifying as well.
     Moreover, the discovered street-names are written to stdout.

Options:
     -v         Verbose: Outputs a lot of additional information on stderr.
     -w         Do not display the graphical window.
     -s         Do not save the <filename>_result.jpg images.
     -p         Do not show additional performance information on stdout.

Internals:
     Street-signs are detected as following:
     * Histogram-matching is used to mark the pixels with a distinct blue street-sign color.
       The positive and negative sample histograms are stored in the posHist.hist and negHist.hist files.
     * Blob detection is employed to segment connected regions of blue pixels into blobs.
     * Blobs with a smaller are than 1/400th of the image and blobs touching the side are pruned.
     * Statistics are generated over the blobs.
       currently: area, roughness, x/y ratio, width / height ratio, orientation, rougness and 'squareness'
     * Blobs are accepted and rejected based on fixed decision boundaries.
     * A convex hull is drawn around positively classified blobs for the result image.
     * Corners of the convex-hull are found by a OpenCV good-features-to-track algorithm.
     * The street-sign is perspective-corrected and cut-out by projecting the four corners on a square
       with the OpenCv cvWarpPerspective function.
     * The resulting sign-image is converted to greyscale over the red channel, and fed to
       the tesseract OCR engine.
     * The OCR result is corrected using a few heuristics to improve final performance.

Output:
     filename:
     streetname1
     streetname2
     ...

Performance Measurement:
     * _mask.png images contain hand-labeled binary mask-images, marking where street signs are present
       in given Images. These masks can be made using the 'maskMaker' program, located elsewhere in this
       package.
     * If the program can find a _mask.png file for an image, it will automatically try to judge
       whether classification was successfull. false detections, undetected signs, and multiple detections
       of a single sign are reported to the stdout. On termination of the program, aggregate statistics are
       displayed.
     * If the program can find a .txt file for an image, it will automatically try to judge whether OCR was
       succesfull. Statistics are aggregated over each detected street-sign. Counts / percentages of entirely
       correctly read street-signs are reported, as well as average edit-distance (levenshtein distance).

Compile:
     type 'make'
     * The OpenCV Open Computer Vision library is a requirement.
       http://opencvlibrary.sourceforge.net/
     * To read the street-signs as well, tesseract must be installed
       with the dutch (NLD) language file.
       http://code.google.com/p/tesseract-ocr/

Files:
     signFinder reads the posHist.hist negHist.hist files for the positive and negative color histograms
     respectively. The software searches for these files in the current directory, and will need these
     files to function correctly.

License:
     All files in this directory and the modules/ subdirectory are licensed
     under a triple MPL 1.1/GPL 2.0/LGPL 2.1 license.
     files in the subdirectories might have different licenses.

See also:
     trainer
     tester
```