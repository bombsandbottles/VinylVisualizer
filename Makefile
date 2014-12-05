#
# Makefile
#
# Remove -D__MACOSX_CORE__ if you're not on OS X
CC      = gcc -g -D__MACOSX_CORE__ -Wno-deprecated
CFLAGS  = -std=c99 -Wall
LIBS = -lportaudio -lsndfile -lncurses -framework OpenGL -framework GLUT -lsamplerate

EXE  = VinylVisualizer

SRCS = vinylVisualizer.c

all: $(EXE)

$(EXE): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf $(EXE).dSYM 
