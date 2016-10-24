#ifndef KVSERVER_H
#define KVSERVER_H

#include "../libs/KVCommon.hpp"
#include "KVServerThread.hpp"

using namespace std;

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
		int MAX_CONNECTIONS;
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Sockets
		int primary_socket;
		int * sockets;
		int max_connection;

		// Buffer
		char * buffer;

		// Managing Sockets
		timeval sockets_last_modified;

		// Available sockets
		vector<KVSocket> socket_vector;
		vector<int> sockets_to_close;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

public:
		KVServer();
		void start();
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);
		void queueSocketToClose(int);

		// Our KV cache
		static KVCache * cache;

};

#endif