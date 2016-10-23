#ifndef KVCLIENT_H
#define KVCLIENT_H

#include "../libs/KVCommon.cpp"
#include "KVClientThread.cpp"

using namespace std;

class KVClient
{
	private:

		// UI Methods
		void runProgram();
		bool runUI();
		char showMenu();
		void viewThreads();

		// Thread worker functions
		void * spawnThreads(KVConnectionDetails *, int, long);
		static void * startClientThread(void *);

		// Keep track of the threads
		vector<pthread_t> threads;

	public:
		KVClient();
		void start(string, int, int, int);
};

#endif