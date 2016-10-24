#ifndef KVSERVER_H
#define KVSERVER_H

#include "../libs/KVCommon.cpp"
#include "KVServerThread.cpp"

// Hashtable
#include <unordered_map>

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

		unordered_map<string, string> hashtable;

		bool get_value(const string&, string&);
        bool createResponseJson(string, string, string, int);

public:
		KVServer();
		void start();
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);
		void queueSocketToClose(int);


};

#endif