CXX = g++
CFLAGS = -Wall -g
LDFLAGS = -lcv -lhighgui

TARGETS = maskMaker
OBJECTS = ImageHandler.o maskMaker.o

maskMaker: $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) -c $<

clean:
	rm $(OBJECTS) $(TARGETS)
