#ifndef KVCLIENTTHREAD_H
#define KVCLIENTTHREAD_H

using namespace std;

class KVClientThread
{
	private:
		
		// Make a connection to the server
		bool makeConnection(string, int);

		// Keep track of the server connection & details
		int server_socket;
		KVConnectionDetails * details;

        void sendMessageToSocket(string, int);

        string createRequestJson(string, string, string);

public:
		KVClientThread(KVConnectionDetails *);
		void sendRequests();

};

#endif