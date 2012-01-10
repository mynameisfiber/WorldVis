GLLIBS = -L/usr/X11/lib -lglut -lGLU -lGL -lXmu -lXext -lXi -lX11 -ljson 
GLINCL = -I/usr/X11/include 
GOPTS  = -std=c++0x -O3 -ffast-math 

default: vis

vis: vis.cpp elements.h
	/usr/local/bin/g++ -o $@ $^ $(GOPTS) $(GLLIBS) $(GLINCL)
