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

KVResult_t KVApi::get(string key)
{
	// Generate the request
	string request  = createRequestJson("GET", key, "", -1);
	string response = send(request);

	if (response.empty())
	{
		KVResult_t bad_result;
		bad_result.err = 400;
		return bad_result;
	}

	return parseResponse(response);
}

KVResult_t KVApi::post(string key, string value, int version)
{
	// Generate the request
	string request  = createRequestJson("POST", key, value, version);
	string response = send(request);

	if (response.empty())
	{
		KVResult_t bad_result;
		bad_result.err = 400;
		return bad_result;
	}

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
	int attempts = 1;
	string response;

	while (attempts++ < 10 && response.empty())
	{
		sendMessageToSocket(request, server_socket);
		response = listenForActivity();

		// Validate the response
		if (!response.empty())
		{
			json inputJson;
			try
			{
				inputJson = json::parse(response);

				int code;
				if (inputJson["code"].is_string())
					code = stoi(inputJson["code"].get<string>());
				else if (inputJson["code"].is_number())
					code = inputJson["code"].get<int>();

				// If the request was invalid, try again
				if (code == 400)
				{
					response = "";
				}
			}
			catch (exception e)
			{
				// idk
				response = "";
			}
		}
	}

	if (attempts >= 10)
	{
		perror("System-wide timeout occurred.");
	}

	return response;
}

KVResult_t KVApi::parseResponse(string s)
{
	// Parse the string
	json inputJson;
	inputJson = json::parse(s);

	string key, value;
	int code = 0, version = 0;

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

	if (inputJson["version"].is_string())
		version = stoi(inputJson["version"].get<string>());
	else if (inputJson["version"].is_number())
		version = inputJson["version"].get<int>();

	KVResult_t * result = new KVResult_t();
	result->key     = key;
	result->value   = value;
	result->err     = code;
	result->version = version;
	return *result;
}

string KVApi::listenForActivity()
{
	if (DEBUG_MODE)
		cout << "... Listening for activity on thread..." << endl;

	// Construct the FD to listen to
	struct pollfd fds[1];
	fds[0].fd     = server_socket;
	fds[0].events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLWRBAND;

	// Wait for activity
	int activity = poll(fds, 1, 0);

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

		// No response
		if (errno == EWOULDBLOCK)
		{
			cout << endl << "Single response timeout." << endl;

			// Return an empty string - this will be flagged for retry
			return "";
		}
		// Read what happened, if anything
		else if (DEBUG_MODE && message_size > 0)
		{
			json request = json::parse(buffer);
			cout << "Recieved response: " << request.dump() << endl;
		}
	}

	return string(buffer);
}

void KVApi::sendMessageToSocket(string request, int socket) {
    //write the message to the server socket
    if (write(socket, request.c_str(), request.length()) < 0){
        perror("Error: could not send message to server");
        exit(1);
    }
}

string KVApi::createRequestJson(string type, string key, string value, int version) {
	json request;
	request["type"] = type;
	request["key"] = key;
	request["value"] = value;
	request["version"] = version;
	return request.dump();
}

bool KVApi::makeConnection(string host, int port)
{
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// Find the server host
	if (getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res) != 0)
	{
		cout << "Error: could not find the host" << endl;
		return false;
	}

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_socket < 0)
	{
		cout << "Error: could not open socket" << endl;
		return false;
	}

	// Timeout logic
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	if (setsockopt (server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		perror("Could not set timeout\r\n");

	if (setsockopt (server_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		perror("Could not set timeout\r\n");

	// Connect to the server
	if (connect(server_socket, res->ai_addr, res->ai_addrlen) < 0) 
	{
		perror("Error: could not connect to host");
		return false;
	}

	return true;
}
