/**
 * Distributed Key-Value Store server class
 */

#include "KVServer.hpp"
#include "../libs/KVCommon.cpp"
#include "../libs/LRU_Cache.hpp"
#include "../libs/KVApi.cpp"
#include "KVServerThread.cpp"

/**
 * Public Methods
 */

// KVCache * KVServer::cache = new KVCache();
cache::LRU_Cache<string, string> * KVServer::cache = new cache::LRU_Cache<string, string>(10);
vector<int> KVServer::sockets_to_close;

int KVServer::BUFFER_SIZE = 513; // Size given in bytes
int KVServer::INCOMING_MESSAGE_SIZE = KVServer::BUFFER_SIZE - 1;

int KVServer::WRITE_QUORUM;
int KVServer::READ_QUORUM;
int KVServer::NUM_NODES;
vector<KVBackendServer_t> KVServer::NODE_ADDRESSES;

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

	// Keep track of performance details
	perf_cp = new vector<string>();

	// Set up the node addresses
	vector< vector<string> > addresses = {
		{"localhost", "56790"},
		{"localhost", "56791"},
		{"localhost", "56792"},
		{"localhost", "56793"},
		{"localhost", "56794"},
		{"localhost", "56795"}
	};

	for (vector< vector<string> >::iterator it = addresses.begin(); it != addresses.end(); ++it)
	{
		// Add a new node
		KVBackendServer_t backend_server;
		backend_server.address = (*it)[0];
		backend_server.port    = (*it)[1];
		backend_server.alive   = true;

		// Add to the active list
		NODE_ADDRESSES.push_back(backend_server);
	}

	// Update our quorums
	KVServer::NUM_NODES    = (int)(NODE_ADDRESSES.size());
	KVServer::WRITE_QUORUM = (int)(NODE_ADDRESSES.size() / 2) + 1;
	KVServer::READ_QUORUM  = (int)(NODE_ADDRESSES.size() / 2);
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
		close(primary_socket);
		exit(1);
	}

	// Avoid the annoying "Address already in use" messages that the program causes
	int opt = 1;
	if (setsockopt(primary_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("Error: could not use setsockopt to set 'SO_REUSEADDR'");
		close(primary_socket);
        exit(1);
    }

	// Async only: set to non-blocking
	if (ASYNC_MODE)
	{
		if (ioctl(primary_socket, FIONBIO, (char *)&opt) < 0)
		{
			perror("Error: ioctl() failed to make primary socket non-blocking");
			close(primary_socket);
			exit(-1);
		}
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

	if (DEBUG_MODE)
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

void KVServer::listenForActivity()
{
	if (DEBUG_MODE)
		KVCommon::clearScreen();

	while (true) 
	{
		// Ready the socket descriptors
		this->resetSocketDescriptors();

		// Determine if we're listening to all ports or just the primary
		int activity;
		if (ASYNC_MODE)
		{
			// Wait for activity - all sockets
			activity = select(max_connection + 1, &socket_descriptors, NULL, NULL, NULL);
		}
		else
		{
			// Wait for activity - only STDIN and primary socket
			// (STDIN usually has 0 reserved, and a custom socket is given a higher number)
			activity = select(primary_socket + 1, &socket_descriptors, NULL, NULL, NULL);
		}

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			if (ASYNC_MODE && (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				cout << "** Notice ** Non-blocking error response." << endl;
				continue;
			}
			else
			{
				perror("Error: select failed");
				exit(1);
			}
		}

		// Handle a key press
		if (FD_ISSET(STDIN_FILENO, &socket_descriptors))
		{
			// Handle the buffer
			char * buffer = new char[BUFFER_SIZE];

			// Clear out the buffer
			memset(&buffer[0], 0, BUFFER_SIZE);

			// Read the incoming message into the buffer
			int message_size = read(STDIN_FILENO, buffer, INCOMING_MESSAGE_SIZE);

			// Allow the user to quit
			if (message_size > 0 && buffer[0] == 'q')
			{
				// Finish closing the sockets
				this->resetSocketDescriptors();

				// Write the performance details to the file
				print_checkpoints_to_file(perf_cp, "s_main");

				close(primary_socket);
				exit(1);
			}
		}
		else if (ASYNC_MODE)
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
		if (DEBUG_MODE)
			cout << "Closed socket FD: " << copy_of_sockets_to_close.front() << endl;
		closeSocket(copy_of_sockets_to_close.front());
		performance_checkpoint(perf_cp, "esocket " + to_string(copy_of_sockets_to_close.front()));
		copy_of_sockets_to_close.erase(copy_of_sockets_to_close.begin());
	}

	// Reset the current socket descriptors
	FD_ZERO(&socket_descriptors);
	FD_SET(primary_socket, &socket_descriptors);

	// Make sure we're listening on STDIN
	FD_SET(STDIN_FILENO, &socket_descriptors);

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
		if (errno == EMFILE || errno == ENFILE)
		{
			// Report connection denied
			cout << "Reached maximum number of clients, denied connection request" << endl;
			return;
		}
		else if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			perror("Error: failure to accept new socket");
			exit(1);
		}
		else
		{
			cout << "Non-blocking error response" << endl;
		}
	}

	// Report new connection
	if (DEBUG_MODE)
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
			//string message = "Server is too busy, please try again later\r\n";
			//write(new_socket, message.c_str(), message.length());

			close(new_socket);
			break;
		}

		// Skip all valid client sockets
		if (sockets[i] != 0) continue;

		// Add new socket
		sockets[i] = new_socket;

		performance_checkpoint(perf_cp, "bsocket " + to_string(new_socket));

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

		performance_checkpoint(perf_cp, "bmessage");
		KVServer::handleMessage(sockets[i]);
		performance_checkpoint(perf_cp, "emessage");
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

	if (message_size < 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			perror("  recv() failed");
			exit(1);
		}
		else
		{
			cout << "Non-blocking error response" << endl;
		}
	}

	// Handle a closed connection
	if (message_size == 0)
	{
		if (DEBUG_MODE)
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
		json request;
		try
		{
			request = json::parse(buffer);
		}
		catch (exception ex)
		{
			string response = KVServer::createResponseJson("INVALID", "", "", 400);
			KVServer::sendMessageToSocket(response, socket_fd);
			return false;
		}

		if (DEBUG_MODE) {
			cout << request.dump(4) << endl;
		}

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
			if (DEBUG_MODE)
				cout << "received Get request";
			string value = string();

			try
			{
				// Search the cache
				value = KVServer::cache->get(key);
				response = KVServer::createResponseJson("GET", key, value, 200);

				// Found the value
				if (DEBUG_MODE)
					cout << endl << "Key found: " << value << endl;
			}
			catch (exception e)
			{
				// Call the backend here
				KVResult_t result = KVServer::backendGet(key);
				response = KVServer::createResponseJson("GET", key, result.value, result.err);

				// If the error is still 404, then that's that..
				if (DEBUG_MODE && result.err == 404)
					cout << endl << "Key Not Found on Backend Server." << endl;
				else if (DEBUG_MODE && result.err == 200)
				{
					cout << endl << "Key Found on Backend Server -> Updating Cache" << endl;

					// Now update the cache
					bool success = KVServer::cache->put(key, result.value);
				}
			}
		}
		else if (requestType == "POST")
		{
			if (DEBUG_MODE)
				cout << "received POST request";

			string value;
			if (request["value"].is_string())
				value = request["value"].get<string>();
			else if (request["value"].is_number())
				value = to_string(request["value"].get<int>());

			// Call the backend here
			KVResult_t result = KVServer::backendPost(key, value);

			// Now update the cache
			bool success = KVServer::cache->put(key, value);

			// Return to the front
			response = KVServer::createResponseJson("POST", key, value, 200);
		}

		// Return the result to the client
		KVServer::sendMessageToSocket(response, socket_fd);
	}

	return true;
}


KVResult_t KVServer::backendGet(string key)
{
	// Keep track of the best result
	KVResult_t latest_result;
	latest_result.version = 0;

	int responses = 0;
	bool read_nodes[NUM_NODES];
	for (int i = 0; i < NUM_NODES; i++)
		read_nodes[i] = false;

	while (responses < READ_QUORUM)
	{
		int node = (int)(rand() % NUM_NODES);
		if (read_nodes[node])
			continue; // Only visit nodes once

		// Update the nodes we just read
		read_nodes[node] = true;

		// Make sure that the node is alive
		if (NODE_ADDRESSES[node].alive == false)
		{
			// Sleep a bit, then give it a try
			usleep(1000); // 1 ms
		}

		// Get the response
		KVResult_t node_result = KVServer::callBackend(NODE_ADDRESSES[node].address, NODE_ADDRESSES[node].port, key, "", -1);

		// Update the best result
		if (latest_result.version < node_result.version)
			latest_result = node_result;

		// If we timeout, then signal that the heartbeat failed
		if (node_result.err == 400)
		{
			// Heartbeat - assume that the node is down.
			NODE_ADDRESSES[node].alive = false;
		}
		else
		{
			// Make sure that this node is treated as alive
			NODE_ADDRESSES[node].alive = true;
			
			// Update the responses retrieved
			responses++;
		}
	}

	return latest_result;
}

KVResult_t KVServer::backendPost(string key, string value)
{
	// First things first.. get the most recent version
	KVResult_t result  = KVServer::backendGet(key);
	int version_number = result.version;

	// Keep track of the best result
	KVResult_t latest_result;
	latest_result.version = 0;

	int responses = 0;
	bool write_nodes[NUM_NODES];
	for (int i = 0; i < NUM_NODES; i++)
		write_nodes[i] = false;

	while (responses < WRITE_QUORUM)
	{
		int node = (int)(rand() % NUM_NODES);
		if (write_nodes[node])
			continue; // Only visit nodes once

		// Update the nodes we just read
		write_nodes[node] = true;

		// Make sure that the node is alive
		if (NODE_ADDRESSES[node].alive == false)
		{
			// Sleep a bit, then give it a try
			usleep(1000); // 1 ms
		}

		// Get the response
		KVResult_t node_result = KVServer::callBackend(NODE_ADDRESSES[node].address, NODE_ADDRESSES[node].port, key, value, version_number);

		// Update the best result
		if (latest_result.version < node_result.version)
			latest_result = node_result;

		// If we timeout, then signal that the heartbeat failed
		if (node_result.err == 400)
		{
			// HEARTBEAT
			// Assume that the node is down.
			NODE_ADDRESSES[node].alive = false;
		}
		else
		{
			// Make sure that this node is treated as alive
			NODE_ADDRESSES[node].alive = true;
			
			// Update the responses retrieved
			responses++;
		}
	}

	return latest_result;
}

KVResult_t KVServer::callBackend(string address, string port, string key, string value, int version)
{
	// Backend calling details
	KVConnectionDetails details;
	details.address = address;
	details.port = stoi(port);

	KVApi * api = new KVApi(&details);
	KVResult_t result;

	// Make the beginning of a call
	//performance_checkpoint(perf_cp, "bbackendcall");

	// Open the connection, send the session's requests
	if (api->open())
	{
		if (DEBUG_MODE)
			cout << "Connection made." << endl;

		if (value.empty())
		{
			result = api->get(key);
		}
		else
		{
			result = api->post(key, value, version);
		}

		api->close();
	}
	else
	{
		cout << "Could not make a connection";
	}

	// Need to sleep at least a little bit, otherwise the Async server
	// could starve threads (currently accessing sockets linearly)
	//usleep(10000); // 10 ms
	//sleep(1);

	//performance_checkpoint(perf_cp, "esession");

	return result;
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
