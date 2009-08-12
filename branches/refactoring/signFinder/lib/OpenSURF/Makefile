# Indicates compilator to use
CC      = g++

# Specifies compilator options
CFLAGS  = -O3 -Wall `pkg-config --cflags opencv` -D LINUX
LDFLAGS = 
LDLIBS  = `pkg-config --libs opencv`

# Files extensions .cpp, .o
SUFFIXES = .cpp .o 
.SUFFIXES: $(SUFFIXES) .

# Name of the main program
PROG  = surf
LIB   = libopensurf.a

# Object files .o necessary to build the main program
OBJS  = fasthessian.o integral.o surf.o utils.o ipoint.o
EXEOBJ= main.o
 
all: $(PROG) $(LIB)

# Compilation and link
$(PROG): $(OBJS) $(EXEOBJ)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(EXEOBJ) $(LDLIBS)

$(LIB): $(OBJS)
	ar ru $(LIB) $(OBJS)
	ranlib $(LIB) 

.cpp.o:
	$(CC)   $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(PROG) $(LIB)
	-rm -f *.o 
