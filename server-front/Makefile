CXX = g++

default: all

all: server

server: server.cpp
	$(CXX) -g -pthread -std=c++0x server.cpp -o server

clean:
	$(RM) server