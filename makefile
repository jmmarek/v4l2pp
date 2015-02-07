all:
	cd v4l2_pp&&make
	cd example&&make

clean:
	rm */*.o
