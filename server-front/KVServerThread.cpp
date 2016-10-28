/**
 * Distributed Key-Value Store server class
 */

#include "KVServerThread.hpp"

using namespace std;
using namespace nlohmann;

/**
 * Public Methods
 */

KVServerThread::KVServerThread(int socket_number)
{
	// Define server limits
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];

	// Keep track of the current socket number
	socket_fd = socket_number;
	printf("Socket number: %d\n", socket_fd);
}

void KVServerThread::start()
{
	// Now listen for updates
	listenForActivity();
}

/**
 * Private Methods
 */

void KVServerThread::listenForActivity()
{
	while (true) 
	{
		cout << "... Listening for activity on thread..." << endl;

		// Construct the FD to listen to
		struct pollfd fds[1];
		fds[0].fd     = socket_fd;
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
			int message_size = read(socket_fd, buffer, INCOMING_MESSAGE_SIZE);

			// Read what happened, if anything
			if (message_size > 0)
			{
				//parse message and execute accordingly
				json request = json::parse(buffer);
				cout << request.dump(4) << endl;

				string key;
				if (request["key"].is_string())
					key = request["key"].get<string>();
				else if (request["key"].is_number())
					key = to_string(request["key"].get<int>());

				string requestType;
				if (request["type"].is_string())
					requestType = request["type"].get<string>();

				string response;
				if (requestType == "GET") {
					cout << "received Get request";
					string value = string();

					bool inTable = KVServer::cache->get_value(key, value);
					if (inTable == false) {
						response = createResponseJson("GET", key, "", 404);
						cout << "Key Not Found.";
					} else {
						response = createResponseJson("GET", key, value, 200);
						cout << value;
					}

				} else if (requestType == "POST") {
					cout << "received POST request";

					string value;
					if (request["value"].is_string())
						value = request["value"].get<string>();
					else if (request["value"].is_number())
						value = to_string(request["value"].get<int>());
					
					bool success = KVServer::cache->post_value(key, value);

					response = createResponseJson("POST", key, value, 200);
				}

				KVServer::cache->print_contents();
			}
			else
			{
				cout << "Need to close the connection for socket FD: " << socket_fd << endl;

				// Close and free the socket
				pthread_mutex_lock(&mutex_sockets_to_close);
				KVServer::sockets_to_close.push_back(socket_fd);
				pthread_mutex_unlock(&mutex_sockets_to_close);

				// Leave!
				return;
			}
		}
	}
}

/**
 * Sending and receiving messages
 */

string KVServerThread::createResponseJson(string type, string key, string value, int code) {
    json response;
    response["type"] = type;
    response["key"] = key;
    response["value"] = value;
    response["code"] = code;
    return response.dump();
}

