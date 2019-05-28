out_dir = ./output

src_dir = ./src

objects = \
	output/diskaccess.o \
	output/utils.o \
	output/myshell.o

all : $(out_dir) main

$(out_dir) :
	mkdir $@

main : $(objects)
	g++ -std=c++11 -g -o $@ $(objects)

output/diskaccess.o : src/structs.h src/error.h src/diskaccess.h src/diskaccess.cpp
	g++ -std=c++11 -g -c src/diskaccess.cpp -o output/diskaccess.o

output/utils.o : src/structs.h src/error.h src/utils.h src/utils.cpp
	g++ -std=c++11 -g -c src/utils.cpp -o output/utils.o

output/myshell.o : src/structs.h src/error.h src/utils.h src/diskaccess.h src/myshell.cpp
	g++ -std=c++11 -g -c src/myshell.cpp -o output/myshell.o

.PHONEY : clean

clean : 
	-rm $(objects) main