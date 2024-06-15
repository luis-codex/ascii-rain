build: rain.cpp
	g++ -m64 -static -o rain rain.cpp -lncurses -ltinfo -ldl -lpthread
run: rain
	./rain