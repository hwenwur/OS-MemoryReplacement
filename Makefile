run: main
	./main

diff: test-result.dat
	diff -Z test-result.dat result3.dat

test-result.dat: main
	./main > test-result.dat

main: main.cpp
	g++ -o main main.cpp
