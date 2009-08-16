CXX = g++
CFLAGS = -Wall -g -I. `pkg-config --cflags opencv` -Ilib/ -Imodules/# -DSHOWIMAGES
LDFLAGS = `pkg-config --libs opencv`

TARGETS = signFinder tester trainer libsignfinder.a
GENOBJ = modules/TestHandler.o lib/bloblib/libblob.a lib/histogramtool/histogramTool.o modules/SignHandler.o modules/CornerFinder.o modules/OCRWrapper.o lib/OpenSURF/libopensurf.a 
LIBOBJECTS = modules/signFinder.o $(GENOBJ)
SIGNOBJECTS = main.o $(LIBOBJECTS) 
TESTOBJECTS = tester.o $(GENOBJ) 
TRAINOBJECTS = trainer.o $(GENOBJ)

all: $(TARGETS)

lib/bloblib/libblob.a:
	cd lib/bloblib; make; cd ../..;

lib/OpenSURF/libopensurf.a:
	cd lib/OpenSURF; make; cd ../..;

libsignfinder.a: $(LIBOBJECTS)
	ar ru $@ $(LIBOBJECTS)
	ranlib $@


signFinder: $(SIGNOBJECTS)
	$(CXX) $(CFLAGS) $(SIGNOBJECTS) $(LDFLAGS) -o $@

tester: $(TESTOBJECTS) 
	$(CXX) $(CFLAGS) $(TESTOBJECTS) $(LDFLAGS) -o $@

trainer: $(TRAINOBJECTS) 
	$(CXX) $(CFLAGS) $(TRAINOBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm $(SIGNOBJECTS) $(TESTOBJECTS) $(TRAINOBJECTS) $(TARGETS)
