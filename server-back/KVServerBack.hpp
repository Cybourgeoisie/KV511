#ifndef KVSERVERBACK_H
#define KVSERVERBACK_H

// Our local cache
#include <regex>
#include "../libs/KVCommon.hpp"

using namespace std;
using namespace nlohmann;

class KVServerBack
{
	private:
		void initialize();
		void resetSocketDescriptors();
		void handleNewConnectionRequest();
		void handleExistingConnections();

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
		KVServerBack();
		void start();
		void listenForActivity();

		// Remove network connections
		void closeSocket(int);
		void queueSocketToClose(int);

		// Messages
		static string createResponseJson(string, string, string, int, int);
		static void sendMessageToSocket(string, int);
		static bool handleMessage(int);

		// Retrieve / Save keys
		static KVResult_t get(string);
		static KVResult_t post(string, string);
		static bool exists(string);

		// Keep track of the sockets that need to be freed
		static vector<int> sockets_to_close;

		// "Constant" values
		static int BUFFER_SIZE;
		static int INCOMING_MESSAGE_SIZE;
		static string STORAGE_FILE;
};

#endif