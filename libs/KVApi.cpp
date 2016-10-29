/**
 * Distributed Key-Value Store client thread class
 */

#include "KVApi.hpp"

using namespace std;
using namespace nlohmann;

/**
 * Public Methods
 */

KVApi::KVApi(KVConnectionDetails * conn_details)
{
	details = conn_details;

	// Define server limits
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];
}

bool KVApi::open()
{
	if (makeConnection(details->address, details->port))
	{
		return true;
	}

	// No connection made
	return false;
}

KVApiResult_t KVApi::get(string key)
{
	// Generate the request
	string request  = createRequestJson("GET", key, "");
	string response = send(request);
	return parseResponse(response);
}

KVApiResult_t KVApi::post(string key, string value)
{
	// Generate the request
	string request  = createRequestJson("POST", key, value);
	string response = send(request);
	return parseResponse(response);
}

bool KVApi::close()
{
	if (server_socket > 0)
	{
		::close(server_socket);
		return true;
	}

	return false;
}


/**
 * Private Methods
 */

string KVApi::send(string request)
{
	string response;
	sendMessageToSocket(request, server_socket);
	listenForActivity(&response);
	return response;
}

KVApiResult_t KVApi::parseResponse(string s)
{
	// Parse the string
	json inputJson;
	inputJson = json::parse(s);

	string key, value;
	int code;

	// Get the key, value, and code
	if (inputJson["key"].is_string())
		key = inputJson["key"].get<string>();
	else if (inputJson["key"].is_number())
		key = to_string(inputJson["key"].get<int>());

	if (inputJson["value"].is_string())
		value = inputJson["value"].get<string>();
	else if (inputJson["value"].is_number())
		value = to_string(inputJson["value"].get<int>());

	if (inputJson["code"].is_string())
		code = stoi(inputJson["code"].get<string>());
	else if (inputJson["code"].is_number())
		code = inputJson["code"].get<int>();

	KVApiResult_t * result;
	result->key   = key;
	result->value = value;
	result->err   = code;
	return *result;
}

void KVApi::listenForActivity(string * response)
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
			*response = string(buffer);
			json request = json::parse(buffer);
			cout << "Recieved response: " << request.dump() << endl;
		}
	}
}

void KVApi::sendMessageToSocket(string request, int socket) {
    //write the message to the server socket
    if (write(socket, request.c_str(), request.length()) < 0){
        perror("Error: could not send message to server");
        exit(1);
    }
}

string KVApi::createRequestJson(string type, string key, string value) {
	json request;
	request["type"] = type;
	request["key"] = key;
	request["value"] = value;
	return request.dump();
}

bool KVApi::makeConnection(string host, int port)
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
}