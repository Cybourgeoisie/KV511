#ifndef KVSERVERTHREAD_H
#define KVSERVERTHREAD_H

using namespace std;

// Mutexes
//static pthread_mutex_t mutex_cache = PTHREAD_MUTEX_INITIALIZER;

class KVServerThread
{
	private:

		// Server limits and port
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Sockets
		int socket_fd;

		// Buffer
		char * buffer;

		// Messages
		string createResponseJson(string, string, string, int);

	public:
		KVServerThread(int);

		void start();
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);

};

#endif