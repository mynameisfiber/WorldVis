GLLIBS = -L/usr/X11/lib -lglut -lGLU -lGL -lXmu -lXext -lXi -lX11 -ljson -L/Developer/SDKs/MacOSX10.7.sdk/usr/lib
GLINCL = -I/usr/X11/include -I/Developer/SDKs/MacOSX10.7.sdk/usr/include
GOPTS  = -std=c++0x -O3 -ffast-math -static-libgcc -static-libstdc++

default: vis

vis: vis.cpp elements.h
	g++ -o $@ $^ $(GOPTS) $(GLLIBS) $(GLINCL)
