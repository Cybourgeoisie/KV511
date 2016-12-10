//#include <csignal>
#include <iostream>
#include "KVServerBack.cpp"

using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "KV Server Backend - startup requested" << endl;

	// Start up the server
	KVServerBack serverback;
	serverback.start();

	return 0;
}
