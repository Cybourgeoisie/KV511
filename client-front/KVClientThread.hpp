#ifndef KVCLIENTTHREAD_H
#define KVCLIENTTHREAD_H

using namespace std;

class KVClientThread
{
	private:
		
		// Server limits and port
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Buffer
		char * buffer;

		// Make a connection to the server
		bool makeConnection(string, int);

		// Keep track of the server connection & details
		int server_socket;
		KVConnectionDetails * details;

        void sendMessageToSocket(string, int);
		void listenForActivity();

        string createRequestJson(string, string, string);

public:
		KVClientThread(KVConnectionDetails *);
		void sendRequests();

};

#endif