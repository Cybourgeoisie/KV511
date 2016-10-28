#ifndef KVSERVER_H
#define KVSERVER_H

// Our local cache
#include "../libs/KVCommon.hpp"
#include "../libs/KVCache.hpp"
#include "KVServerThread.hpp"

using namespace std;

// Mutexes
static pthread_mutex_t mutex_sockets_to_close = PTHREAD_MUTEX_INITIALIZER;

class KVServer
{
	private:
		void initialize();
		void runProgram();
		void resetSocketDescriptors();
		void handleNewConnectionRequest();
		void handleExistingConnections();
		static void * startServerThread(void *);

		// Own socket
		void openSocket();
		int  bindSocket(int);

		// Server limits and port
		int PORT_NUMBER;
		int MAX_SESSIONS;
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Sockets
		int primary_socket;
		int * sockets;
		int max_connection;

		// Buffer
		char * buffer;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

public:
		KVServer();
		void start(bool use_async);
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);
		void queueSocketToClose(int);

		// Our KV cache
		static KVCache * cache;

		// Keep track of the sockets that need to be freed
		static vector<int> sockets_to_close;
};

#endif