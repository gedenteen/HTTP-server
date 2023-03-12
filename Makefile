.PHONY: all clean

# linking (make binary execuatable file)
all: src/main.o src/libmyserver.so
	gcc src/main.o -o main.bin -L./src -lmyserver -Wl,-rpath,./src

# complilation main.c
src/main.o: src/main.c
	gcc src/main.c -c -o src/main.o

# creating shared library 
src/libmyserver.so: src/server.o
	gcc -shared src/server.o -o src/libmyserver.so

# compilation servec.c for creating shared library 
src/server.o: src/server.c
	gcc src/server.c -fPIC -c -o src/server.o

clean:
	rm -rf src/*.o src/*.so *.bin
