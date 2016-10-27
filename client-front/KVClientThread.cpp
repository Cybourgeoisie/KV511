/**
 * Distributed Key-Value Store client thread class
 */

#include "KVClientThread.hpp"

using namespace std;

/**
 * Public Methods
 */

KVClientThread::KVClientThread(KVConnectionDetails * conn_details)
{
	details = conn_details;
}

void KVClientThread::sendRequests()
{
	// Make the connection - if not possible, thread will just exit
	if (makeConnection(details->address, details->port))
	{
		cout << "Connection made." << endl;
        auto thread_id = std::this_thread::get_id();
        stringstream ss;
        ss << thread_id;
        string fileName = ss.str();
        string sessionTxt = "_Session";
        fileName.append(sessionTxt);

        string command = "python ../libs/jsonGenReq.py ";
        command.append(to_string(SESSION_LENGTH));
        command.append(" ");
        command.append(fileName);
        cout << command << endl;
        system(command.c_str());
		nlohmann::json inputJson;
		inputJson = nlohmann::json::parse(get_file_contents(fileName.c_str()));

		long i = 0;
		while (1)
		{
			for (nlohmann::json::iterator it = inputJson.begin(); it != inputJson.end(); ++it) {
				sendMessageToSocket(*it, server_socket);
				sleep(5);
			}
			//string request = createRequestJson("POST", std::to_string(i), "World!");
            //sendMessageToSocket(request, server_socket);
            //sleep(5);
			//i += 1;
		}
	}
}

void KVClientThread::sendMessageToSocket(string request, int socket) {
    //write the message to the server socket
    if (write(socket, request.c_str(), request.length()) < 0){
        perror("Error: could not send message to server");
        exit(1);
    }
}

string KVClientThread::createRequestJson(string type, string key, string value) {
	nlohmann::json request;
	request["type"] = type;
	request["key"] = key;
	request["value"] = value;
	return request.dump();
}


/** 
 * Private Methods
 */

/**
 * Make a connection
 */
bool KVClientThread::makeConnection(string host, int port)
{
	// Debug
	printf("Connecting to %s:%d\n", details->address.c_str(), details->port);

	struct sockaddr_in server_address;
	struct hostent * server;

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		cout << "Error: could not open socket" << endl;
		return false;
	}

	// Find the server host
	server = gethostbyname(host.c_str());
	if (server == NULL)
	{
		cout << "Error: could not find the host" << endl;
		return false;
	}

	// Clear out the server_address memory space
	memset((char *) &server_address, 0, sizeof(server_address));

	// Configure the socket information
	server_address.sin_family = AF_INET;
	memcpy(server->h_addr, &server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(port);

	// Connect to the server
	if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) 
	{
		perror("Error: could not connect to host");
		return false;
	}

	return true;

	cout << "Connected to server on port " << port << endl;
}
