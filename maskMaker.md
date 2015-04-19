
```

     maskMaker - Allows user to hand-label parts of images and generate binary masks of labeled parts.

Usage:
     maskMaker <image-file.jpg ...>

Description:
     This simple tool allows the user to hand-label parts of an image. There are two tools available
     for this. The default 'polygon' tool is convenient for marking rectangular objects.
     the tool waits until the user has clicked on four corners of an object, marking the area between the
     four clicks. The 'fill' tool can be toggled with the 't' key, and fills an area which is similarly colored.
     Marked areas are increased in brightness. results are saved to a _result.png file.
     If a mask is already present for a certain image, this mask is immediately displayed.

keys:
     n - next image.
     p - previous image.
     t - switch tool.
     + - increase fill-tool threshold.
     - - decrease fill-tool threshold.

Compile:
     type 'make'
     The OpenCV Open Computer Vision library is a requirement.
     http://opencvlibrary.sourceforge.net/

License:
     All files in this directory are licensed under a triple MPL 1.1/GPL 2.0/LGPL 2.1 license.
     files in the subdirectories might have different licenses.

Available on: http://code.google.com/p/signfinder/
06/29/2009 - Tijs Zwinkels
```