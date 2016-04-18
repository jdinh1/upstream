# cs335 project
# to compile your project, type make and press enter

all: upstream

upstream: upstream.cpp
	g++ upstream.cpp ppm.cpp log.cpp -Wall -o upstream libggfonts.a -lX11 -lGL -lGLU -lrt -pthread -lm

clean:
	rm -f upstream
	rm -f *.o

