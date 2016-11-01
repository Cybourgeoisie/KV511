CXX = g++

default: all

all: server

server: server.cpp
	$(CXX) -pg -pthread -std=c++0x server.cpp -o server.o

clean:
	$(RM) -r server.o server.o.dSYM