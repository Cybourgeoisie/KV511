/**
 * Distributed Key-Value Store server class
 */

#include "KVServer.hpp"
#include "../libs/KVCommon.cpp"
#include "../libs/KVCache.cpp"
#include "KVServerThread.cpp"

/**
 * Public Methods
 */

KVCache * KVServer::cache = new KVCache();
vector<int> KVServer::sockets_to_close;

int KVServer::BUFFER_SIZE = 513; // Size given in bytes
int KVServer::INCOMING_MESSAGE_SIZE = KVServer::BUFFER_SIZE - 1;

// Be aware of the global var provided by server.cpp
extern bool ASYNC_MODE;

KVServer::KVServer()
{
	// Define the port number to listen through
	PORT_NUMBER = 56789;

	// Define server limits
	MAX_SESSIONS = 512;

	// Keep track of the client sockets
	sockets = new int[MAX_SESSIONS];
}

void KVServer::start(bool use_async)
{
	// Determine if we want to use MT
	ASYNC_MODE = use_async;

	if (ASYNC_MODE)
		cout << "Running server in Async mode." << endl;
	else
		cout << "Running server in MT mode." << endl;

	// Open the socket and listen for connections
	initialize();
	openSocket();

	// Now listen for updates
	listenForActivity();
}

void * KVServer::startServerThread(void * arg)
{
	KVServerThread * thread = new KVServerThread((long) arg);
	thread->listenForActivity();
	pthread_exit(NULL);
}

/**
 * Private Methods
 */

void KVServer::initialize()
{
	// Ensure that all sockets are invalid to boot (if using MT)
	for (int i = 0; i < MAX_SESSIONS; i++)
	{
		sockets[i] = 0;
	}
}


/**
 * ::: Multi-threaded version of the server :::
 */

/**
 * Primary Socket
 * - Open and bind a socket to serve
 */

void KVServer::openSocket()
{
	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	primary_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (primary_socket < 0)
	{
		perror("Error: could not open primary socket");
		exit(1);
	}

	// Avoid the annoying "Address already in use" messages that the program causes
	int opt = 1;
	if (setsockopt(primary_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("Error: could not use setsockopt to set 'SO_REUSEADDR'");
        exit(1);
    }

	// Bind to the given socket
	int socket_status = this->bindSocket(primary_socket);
	if (socket_status < 0)
	{
		perror("Fatal Error: Could not bind primary socket to any ports");
		exit(1);
	}

	// Start listening on this port - second arg: max pending connections
	if (listen(primary_socket, MAX_SESSIONS) < 0)
	{
		perror("Error: could not listen on port");
		exit(1);
	}

	cout << "Listening on port " << PORT_NUMBER << endl;
}

int KVServer::bindSocket(int socket)
{
	struct sockaddr_in server_address;

	// Clear out the server_address memory space
	memset((char *) &server_address, 0, sizeof(server_address));

	// Configure the socket information
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT_NUMBER);

	return ::bind(socket, (struct sockaddr *) &server_address, sizeof(server_address));
}

/**
 *
 * TODO - Now that we have support for MT and ST, we need to make the ST
 *        version asynchronous. And then event-based support.
 *
 **/

void KVServer::listenForActivity()
{
	KVCommon::clearScreen();

	while (true) 
	{
		// Ready the socket descriptors
		this->resetSocketDescriptors();

		// Determine if we're listening to all ports or just the primary
		int activity;
		if (ASYNC_MODE)
		{
			// Wait for activity
			activity = select(max_connection + 1, &socket_descriptors, NULL, NULL, NULL);
		}
		else
		{
			// Listen only on the primary socket
			struct pollfd fds[1];
			fds[0].fd     = primary_socket;
			fds[0].events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLWRBAND;

			// Wait for activity
			activity = poll(fds, 1, NULL);
		}

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			perror("Error: select failed");
			exit(1);
		}

		// Handle a new connection
		if (ASYNC_MODE)
		{
			// Anything on the primary socket is a new connection
			if (FD_ISSET(primary_socket, &socket_descriptors))
			{
				this->handleNewConnectionRequest();
			}

			// Perform any open activities on all other clients
			this->handleExistingConnections();
		}
		else
		{
			// In MT mode, the primary only handles new connection requests
			this->handleNewConnectionRequest();
		}
	}
}

void KVServer::sendMessageToSocket(string request, int socket) {
    //write the message to the client socket
    if (write(socket, request.c_str(), request.length()) < 0){
        perror("Error: could not send message to client");
        exit(1);
    }
}

string KVServer::createResponseJson(string type, string key, string value, int code) {
    json response;
    response["type"] = type;
    response["key"] = key;
    response["value"] = value;
    response["code"] = code;
    return response.dump();
}


/**
 * Handle Connection Activity
 */

void KVServer::resetSocketDescriptors()
{
	// Determine if we have any sockets to close
	vector<int> copy_of_sockets_to_close;

	if (ASYNC_MODE)
	{
		// For (1) fair comparison with MT, and (2) to keep code cleaner
		copy_of_sockets_to_close = KVServer::sockets_to_close; // Make a copy of the data
		KVServer::sockets_to_close.clear(); // Empty the sockets to close
	}
	else
	{
		// Trying to do as little work as possible here... Just copy and clear.
		pthread_mutex_lock(&mutex_sockets_to_close);
		copy_of_sockets_to_close = KVServer::sockets_to_close; // Make a copy of the data
		KVServer::sockets_to_close.clear(); // Empty the sockets to close
		pthread_mutex_unlock(&mutex_sockets_to_close);
	}

	// Close those sockets ready to close.
	while (copy_of_sockets_to_close.size() > 0)
	{
		cout << "Closed socket FD: " << copy_of_sockets_to_close.front() << endl;
		closeSocket(copy_of_sockets_to_close.front());
		copy_of_sockets_to_close.erase(copy_of_sockets_to_close.begin());
	}

	// Reset the current socket descriptors
	FD_ZERO(&socket_descriptors);
	FD_SET(primary_socket, &socket_descriptors);

	// Keep track of the maximum socket descriptor for select()
	max_connection = primary_socket;

	// Add remaining sockets
	for (int i = 0; i < MAX_SESSIONS; i++)
	{
		// Validate the socket
		if (sockets[i] <= 0) continue;

		// Add socket to set
		FD_SET(sockets[i], &socket_descriptors);

		// Update the maximum socket descriptor
		max_connection = max(max_connection, sockets[i]);
	}
}

void KVServer::handleNewConnectionRequest()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Accept a new socket
	int new_socket = accept(primary_socket, (struct sockaddr *)&client_address, &client_address_length);

	// Validate the new socket
	if (new_socket < 0)
	{
		perror("Error: failure to accept new socket");
		exit(1);
	}

	// Report new connection
	cout << "New Connection Request: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

	// Add socket to open slot
	for (int i = 0; i <= MAX_SESSIONS; i++)
	{
		// If we reach the maximum number of clients, we've gone too far
		// Can't accept a new connection!
		if (i == MAX_SESSIONS)
		{
			// Report connection denied
			cout << "Reached maximum number of clients, denied connection request" << endl;

			// Send refusal message to socket
			string message = "Server is too busy, please try again later\r\n";
			write(new_socket, message.c_str(), message.length());

			close(new_socket);
			break;
		}

		// Skip all valid client sockets
		if (sockets[i] != 0) continue;

		// Add new socket
		sockets[i] = new_socket;

		// Only need to worry about MT here - async's work is set with the sockets[] array
		if (!ASYNC_MODE)
		{
			// MT - create a new thread, pass off the socket to it
			pthread_t node_thread;
			if (pthread_create(&node_thread, NULL, &KVServer::startServerThread, (void *) new_socket) != 0)
			{
				perror("Error: could not spawn peer listeners");
				exit(1);
			}			
		}

		break;
	}
}

void KVServer::handleExistingConnections()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Iterate over all clients
	for (int i = 0; i < MAX_SESSIONS; i++) 
	{
		if (!FD_ISSET(sockets[i], &socket_descriptors)) continue;

		KVServer::handleMessage(sockets[i]);
	}
}

bool KVServer::handleMessage(int socket_fd)
{
	// Handle the buffer
	char * buffer = new char[BUFFER_SIZE];

	// Clear out the buffer
	memset(&buffer[0], 0, BUFFER_SIZE);

	// Read the incoming message into the buffer
	int message_size = read(socket_fd, buffer, INCOMING_MESSAGE_SIZE);

	// Handle a closed connection
	if (message_size == 0)
	{
		cout << "Need to close the connection for socket FD: " << socket_fd << endl;

		if (ASYNC_MODE)
		{
			KVServer::sockets_to_close.push_back(socket_fd);
		}
		else
		{
			pthread_mutex_lock(&mutex_sockets_to_close);
			KVServer::sockets_to_close.push_back(socket_fd);
			pthread_mutex_unlock(&mutex_sockets_to_close);
		}

		return false;
	}
	else
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
		if (requestType == "GET")
		{
			cout << "received Get request";
			string value = string();

			bool inTable = KVServer::cache->get_value(key, value);

			if (inTable == false) {
				response = KVServer::createResponseJson("GET", key, "", 404);
				cout << endl << "Key Not Found." << endl;
			} else {
				response = KVServer::createResponseJson("GET", key, value, 200);
				cout << endl << "Key found: " << value << endl;
			}
		}
		else if (requestType == "POST")
		{
			cout << "received POST request";

			string value;
			if (request["value"].is_string())
				value = request["value"].get<string>();
			else if (request["value"].is_number())
				value = to_string(request["value"].get<int>());
			
			bool success = KVServer::cache->post_value(key, value);

			response = KVServer::createResponseJson("POST", key, value, 200);
		}

		// Return the result to the client
		KVServer::sendMessageToSocket(response, socket_fd);

		KVServer::cache->print_contents();
	}

	return true;
}


/**
 * Handle closing sockets
 */

void KVServer::closeSocket(int socket)
{
	// Remove from the int array
	for (int i = 0; i < MAX_SESSIONS; i++)
	{
		if (socket == sockets[i])
		{
			sockets[i] = 0;
			break;
		}
	}

	// Close and free the socket
	close(socket);
}

