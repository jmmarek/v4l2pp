FILES = window.o

LGTKMM = `pkg-config --cflags --libs gtkmm-3.0`

CC = g++

CPPFLAGS = -std=c++11 -g -Wall

all: main.cpp $(FILES)
	g++ $(FILES) main.cpp -o v4l2_example $(CPPFLAGS)  -L. ../v4l2_pp/libv4l2_camera.so.1 -lpthread $(LGTKMM)

%.o : %.cpp
	$(CC) -c $(CPPFLAGS) $(LGTKMM) $< -o $@ 

#To run program, shared library has to be found in shared library folder (on Debian Wheezy it's /usr/lib), or you can link it using following command:
#LD_LIBRARY_PATH=/path/to/library/
#export=LD_LIBRARY_PATH

clean:
	rm *.o
