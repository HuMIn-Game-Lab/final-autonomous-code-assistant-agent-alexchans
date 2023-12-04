compile:
	clang++ -o app ./Code/*.cpp -L./Code/lib -ljob

libLinux:
	clear
	clang++ -shared -o ./Code/lib/libjob.so -fPIC ./Code/lib/*.cpp

buildLinux:
	clear
	clang++ -o app ./Code/*.cpp -L./Code/lib -ljob -Wl,-rpath=./Code/lib
run:
	clear
	./app
