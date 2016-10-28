/**
 * Distributed Key-Value Store client thread class
 */

#include "KVClientThread.hpp"

using namespace std;
using namespace nlohmann;

/**
 * Public Methods
 */

KVClientThread::KVClientThread(KVConnectionDetails * conn_details)
{
	details = conn_details;

	// Define server limits
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];
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
        string threadId = ss.str();
        string fileName = threadId;
        string sessionTxt = "_Session";
        fileName.append(sessionTxt);

        string command = "python ../libs/jsonGenReq.py ";
        command.append(to_string(SESSION_LENGTH));
        command.append(" ");
        command.append(fileName);
        cout << "Generated filename: " << command << endl;

        // Run the Python script
        system(command.c_str());

        // Pull back in the created JSON
		json inputJson;
		inputJson = json::parse(get_file_contents(fileName.c_str()));

		// Send all of the messages
		for (json::iterator it = inputJson.begin(); it != inputJson.end(); ++it)
		{
			// Send the message
			sendMessageToSocket((*it).dump(), server_socket);

			// Currently, this will BLOCK
			// It waits for the response from the server.
			// If we want to flood the server with non-blocking requests, 
			// then just comment this out. And, of course, remove the sleep()
			listenForActivity();

			sleep(1);
		}

		cout << "-- Thread ID " << threadId << " finished --" << endl;
	}
}

void KVClientThread::listenForActivity()
{
	cout << "... Listening for activity on thread..." << endl;

	// Construct the FD to listen to
	struct pollfd fds[1];
	fds[0].fd     = server_socket;
	fds[0].events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLWRBAND;

	// Wait for activity
	int activity = poll(fds, 1, NULL);

	// Validate the activity
	if ((activity < 0) && (errno!=EINTR))
	{
		perror("Error: select failed");
		exit(1);
	}
	else
	{
		// Clear out the buffer
		memset(&buffer[0], 0, BUFFER_SIZE);

		// Read the incoming message into the buffer
		int message_size = read(server_socket, buffer, INCOMING_MESSAGE_SIZE);

		// Read what happened, if anything
		if (message_size > 0)
		{
			json request = json::parse(buffer);
			cout << "Recieved response: " << request.dump() << endl;
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
	json request;
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
