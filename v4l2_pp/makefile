CC = g++

CPPFLAGS = -std=c++11 -fpic -lv4l2

v4l2_camera.o : v4l2_camera.cpp v4l2_camera.h
	$(CC) -shared $(CPPFLAGS) -Wl,-soname,libv4l2_camera.so.1 -o libv4l2_camera.so.1 v4l2_camera.cpp -lz

clean:
	rm *.o
#To install this library make this:
#cp libv4l2_camera.so.1 /usr/lib/
