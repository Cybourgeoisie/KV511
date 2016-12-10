CXX = g++

default: all

all: server-back

server-back: server-back.cpp
	$(CXX) -pg -std=c++0x server-back.cpp -o server-back.o

clean:
	$(RM) -r server-back.o server-back.o.dSYM