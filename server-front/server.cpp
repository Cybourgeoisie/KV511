//#include <csignal>
#include <iostream>
#include "KVServer.cpp"

using namespace std;

// Determine if we're using the multithreaded or the asynch version
// Default - Async disabled, MT enabled
bool ASYNC_MODE = false;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "KV Server - startup requested" << endl;

	// Determine if the user wants to use ASYNC or MT mode
	bool use_async = false;
	if (argc == 2)
	{
		use_async = (atoi(argv[1]) == 1) ? true : false;
	}
	else
	{
		cerr << "Required arguments: <use asynchronous version (1 - use, else - don't use)>" << endl;
		return -1;
	}

	// Start up the server
	KVServer server;
	server.start(use_async);

	return 0;
}
