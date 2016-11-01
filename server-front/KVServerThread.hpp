#ifndef KVSERVERTHREAD_H
#define KVSERVERTHREAD_H

using namespace std;

class KVServerThread
{
	private:

		// Sockets
		int socket_fd;

		// Keep track of performance details
		vector<string> * perf_cp;

	public:
		KVServerThread(int);

		void start();
		void listenForActivity();
};

#endif