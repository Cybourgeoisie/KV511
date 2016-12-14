#ifndef KVSERVER_H
#define KVSERVER_H

// Our local cache
#include "../libs/KVCommon.hpp"
//#include "../libs/KVCache.hpp"
#include "../libs/LRU_Cache.hpp"
#include "../libs/KVApi.hpp"
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

		// Sockets
		int primary_socket;
		int * sockets;
		int max_connection;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

		// Keep track of performance details
		vector<string> * perf_cp;

public:
		KVServer();
		void start(bool use_async);
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);
		void queueSocketToClose(int);

		// Messages
		static string createResponseJson(string, string, string, int);
		static void sendMessageToSocket(string, int);
		static bool handleMessage(int);

		// Calls to the backend
		static KVResult_t callBackend(string, string, string, string, int);
		static KVResult_t backendGet(string);
		static KVResult_t backendPost(string, string);

		// Our KV cache
		// static KVCache * cache;
		// Now a LRU cache
		static cache::LRU_Cache<string, string> * cache;

		// Keep track of the sockets that need to be freed
		static vector<int> sockets_to_close;

		// "Constant" values
		static int BUFFER_SIZE;
		static int INCOMING_MESSAGE_SIZE;

		// Qurorum Values
		static vector<KVBackendServer_t> NODE_ADDRESSES;
		static int NUM_NODES;
		static int READ_QUORUM;
		static int WRITE_QUORUM;
};

#endif