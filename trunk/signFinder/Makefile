CXX = g++
CFLAGS = -Wall -Winvalid-pch -g -Iopencv/ # -DSHOWIMAGES
LDFLAGS = -lcv -lhighgui 

TARGETS = signFinder tester trainer
GENOBJ = TestHandler.o lib/bloblib/libblob.a lib/histogramtool/histogramTool.o 
SIGNOBJECTS = signFinder.o CornerFinder.o $(GENOBJ) 
TESTOBJECTS = tester.o $(GENOBJ) 
TRAINOBJECTS = trainer.o lib/histogramtool/histogramTool.o

all: $(TARGETS)

lib/bloblib/libblob.a:
	cd lib/bloblib; make; cd ../..;

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
