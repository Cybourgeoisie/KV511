CXX = g++

default: all

all: client

client: client.cpp
	$(CXX) -pg -pthread -std=c++0x client.cpp -o client.o

clean:
	$(RM) -r client.o client.o.dSYM