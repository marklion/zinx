libzinx.so:*.c* *.h
	g++ -std=c++11 -fPIC -shared $^ -o $@
install:
	cp libzinx.so /usr/lib/
	cp *.h /usr/include/
