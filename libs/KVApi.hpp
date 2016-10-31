#ifndef KVAPI_H
#define KVAPI_H

using namespace std;
using namespace nlohmann;

typedef struct {
	int err;
	string key;
	string value;
} KVApiResult_t;

class KVApi
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
		string listenForActivity();

        string createRequestJson(string, string, string);

        string send(string);
        KVApiResult_t parseResponse(string);

	public:

		KVApi(KVConnectionDetails *);

		// API
		bool open();
		KVApiResult_t get(string);
		KVApiResult_t post(string, string);
		bool close();
};

#endif