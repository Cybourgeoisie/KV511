//#include <csignal>
#include <iostream>
#include "KVServer.cpp"

using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "KV Server - startup requested" << endl;

	// Start up the server
	KVServer server;
	server.start();

	cout << "test" << endl;

	return 0;
}
