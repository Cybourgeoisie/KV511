//#include <csignal>
#include <iostream>
#include "KVServerBack.cpp"

using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "KV Server Backend - startup requested" << endl;

	// Determine if the user wants to use ASYNC or MT mode
	int port;
	if (argc == 2)
	{
		port = atoi(argv[1]);
	}
	else
	{
		cerr << "Required arguments: <port number>" << endl;
		return -1;
	}

	// Start up the server
	KVServerBack serverback;
	serverback.start(port);

	return 0;
}
