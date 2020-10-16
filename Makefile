run: main
	./main

diff: pageout.dat
	diff -Z pageout.dat result3.dat

pageout.dat: main
	./main "test3dat.dat"

main: main.cpp
	g++ -o main main.cpp
