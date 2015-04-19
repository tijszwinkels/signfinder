# Introduction #

Right now, the signFinder is specifically trained and designed to read Dutch street-signs.
However, a toolset to train the software is provided. In this way, the software can easily retrained to read streetsigns from another country, or to read streetsign-like signs such as license-plates.

# Procedure Overview #

  * Take many pictures of street-signs.
  * Separate 10% from your pictures into another directory as test-set.
  * Label the street-signs in the pictures with the 'maskMaker' utility.
  * Train the labeled street-signs with the 'trainer' utility
  * Optionally: use the 'tester' utility to determine a proper threshold.

# Gathering Data #

The software needs to be trained on a wide and diverse selection of street-signs pictures. Right now, mainly the color of the signs is important, but other features (such as the SURF keypoints that are already being extracted) might be important in the near future as well. In order to generalize well, the software must 'see' a wide range of street-signs, ideally in several different stages of aging, and with different lighting and camera models. Often diversity is more important than quantity. It's good to realize that these kind of learning algorithms learn to recognize what you teach them, nothing more and nothing less. If you go out and train the software on 80 pictures that are taken in a modern residential area, that will be the kind of signs that it will recognize reliably.
Instead of taking the pictures by yourself, it might be worth to see whether pictures of street signs with an appropriate license can be harvested from the internet. Try [flickr](http://www.flickr.com/search/?q=street+sign&l=deriv&ss=0&ct=0&mt=all&w=all&adv=1).


## quantity ##
The databases that comes with this software have been trained on about 175 pictures of street signs. This seems to offer reasonable performance given that the street signs to be detected are not too rusted or discolored.  An earlier version had been trained on about 75 pictures of (mostly similar) street signs, but generalization performance was not too good. In general, aim for at least fifty diverse pictures.

## other items ##
All pixels that aren't labeled as being a street-sign, are taken to be a negative sample. This means that any pixel that has the same color as a street-sign but isn't labeled as such, will actually decrease the likelihood that street-signs are detected correctly. For this reason, it's often a good idea to not use pictures that contains other traffic-signs.

## Blue street signs? ##
Does your country have blue street signs, like the Netherlands? Or just want to work on improving the Dutch street sign detector?
The detector might just work out of the box for you. If not, you might still be able to benefit from the dutch training-data, it's available on: http://mirror.openstreetmap.nl/openstreetphoto/tijs/dataset/trainset/

## White street signs? ##
Many other objects in a typical street-environment are white, not to mention a cloudy sky. Since right now only colors are used to segment street signs, I don't expect that the software will be able to segment white street signs correctly. Sorry.

# Labeling Data #
Have you gathered a sufficiently large and diverse dataset with pictures of street signs? good! - Now we have to tell the software which parts of the image contain street signs and which parts don't. This can be done with the '[maskMaker](maskMaker.md)' program.
Use as following:
  * load the image files on the command line. - For example, if your dataset resides in ~/dataset, you can do `./maskMaker ~/dataset/*.jpg`
  * click the four corners of each of the street-signs in the image. After clicking four corners, the labeled part image should get a white layer, as can be seen in the image.
  * press 's' to save the labeled images. - This creates a `<original file name>_mask.png` file containing a binary mask.
  * press 'n' to go to the next image. Don't forget to save first.
  * press 'q' to quit.
If you're not satisfied with the region that you've labeled, you can press 'z' to undo the last change.

Remember, all pixels that aren't labeled as being a street-sign, are taken to be a negative sample. This means that any pixel that has the same color as a street-sign but isn't labeled as such, will actually decrease the likelihood that street-signs are detected correctly. For this reason, it's often a good idea to not label pictures that contains other traffic-signs. If you don't want a certain image to be used by the trainer, just don't save the label.
Under no circumstance save the label without any signs marked, as this will cause the entire image to be considered a negative sample.

![http://mirror.openstreetmap.nl/openstreetphoto/tijs/results/data/Screenshot-MaskMaker.png](http://mirror.openstreetmap.nl/openstreetphoto/tijs/results/data/Screenshot-MaskMaker.png)


# Training the Classifier #
After the dataset has been labeled, the color histograms need to be trained. This can be done with the '[trainer](trainer.md)' program. The syntax is similar to maskMaker, just put the images to train on the command line. The trainer will find the `_mask.png` labels automatically.
  * If you want to train all jpg-files in the ~/Dataset1 directory, you'll do: `./trainer ~/Dataset1/*.jpg`
  * If you want to train all jpg-files in the ~/Dataset1 and ~/Dataset2 directory, you'll do: `./trainer ~/Dataset1/*.jpg ~/Dataset2/*.jpg`

This will create a `_posHist.hist`, a `_negHist.hist`, and a `_surfkeys.dat` file in the current directory. Since the [signFinder](signFinder.md) executable reads the files posHist.hist, negHist.hist and surfkeys.dat, the files will need to be renamed in order to be used by the signFinder.
  * ` mv _postHist.hist posHist.hist `
  * ` mv _negHist.hist negHist.hist `
  * ` mv _surfkeys.dat surfkeys.dat `
From now on, the newly trained database will be used.

# Also See #

In order to understand the performance of [signFinder](signFinder.md) and train it optimally, it's useful to know how the program works. See the [HowSignfinderWorks](HowSignfinderWorks.md) document for more information.