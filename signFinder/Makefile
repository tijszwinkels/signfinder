CXX = g++
CFLAGS = -Wall -g -Iopencv/ # -DSHOWIMAGES
LDFLAGS = -lcv -lhighgui 

TARGETS = signFinder tester
GENOBJ = TestHandler.o lib/bloblib/libblob.a lib/histogramtool/histogramTool.o 
SIGNOBJECTS = signFinder.o $(GENOBJ) 
TESTOBJECTS = tester.o $(GENOBJ) 

all: $(TARGETS)

lib/bloblib/libblob.a:
	cd lib/bloblib; make; cd ../..;

signFinder: $(SIGNOBJECTS)
	$(CXX) $(CFLAGS) $(SIGNOBJECTS) $(LDFLAGS) -o $@

tester: $(TESTOBJECTS) 
	$(CXX) $(CFLAGS) $(TESTOBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm $(SIGNOBJECTS) $(TESTOBJECTS) $(TARGETS)
