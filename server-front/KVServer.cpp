/**
 * Distributed Key-Value Store server class
 */

#include "KVServer.hpp"

/**
 * Public Methods
 */

KVServer::KVServer()
{
	// Define the port number to listen through
	PORT_NUMBER = 56789;

	// Define server limits
	MAX_CONNECTIONS = 512;
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Keep track of the client sockets
	sockets = new int[MAX_CONNECTIONS];

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);
}

void KVServer::start()
{
	// Open the socket and listen for connections
	initialize();
	openSocket();

	// Now listen for updates
	listenForActivity();
}

/*
void * KVServer::startActivityListenerThread(void * arg)
{
	KVServer * node;
	node = (KVServer *) arg;
	node->listenForActivity();
	pthread_exit(NULL);
}
*/

/**
 * Private Methods
 */

void KVServer::initialize()
{
	// Ensure that all sockets are invalid to boot
	for (int i = 0; i < MAX_CONNECTIONS; i++)
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
	if (listen(primary_socket, MAX_CONNECTIONS) < 0)
	{
		perror("Error: could not listen on port");
		exit(1);
	}

	KVSocket a_socket;
	a_socket.socket_id = primary_socket;
	a_socket.type = "primary";
	socket_vector.push_back(a_socket);

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);

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
	KVCommon::clearScreen();

	while (true) 
	{
		cout << "... Listening for activity..." << endl;

		// Ready the socket descriptors
		this->resetSocketDescriptors();
  
		// Wait for activity
		int activity = select(max_connection + 1, &socket_descriptors, NULL, NULL, NULL);

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			perror("Error: select failed");
			exit(1);
		}

		// Anything on the primary socket is a new connection
		if (FD_ISSET(primary_socket, &socket_descriptors))
		{
			this->handleNewConnectionRequest();
		}

		// Perform any open activities on all other clients
		this->handleExistingConnections();
	}
}

/**
 * Handle Connection Activity
 */

void KVServer::resetSocketDescriptors()
{
	// Determine if we have any sockets to close
	while (sockets_to_close.size() > 0)
	{
		closeSocket(sockets_to_close.front());
		sockets_to_close.erase(sockets_to_close.begin());
	}

	// Reset the current socket descriptors
	FD_ZERO(&socket_descriptors);
	FD_SET(primary_socket, &socket_descriptors);

	// Keep track of the maximum socket descriptor for select()
	max_connection = primary_socket;

	// Add remaining sockets
	for (int i = 0; i < MAX_CONNECTIONS; i++)
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
	for (int i = 0; i <= MAX_CONNECTIONS; i++)
	{
		// If we reach the maximum number of clients, we've gone too far
		// Can't accept a new connection!
		if (i == MAX_CONNECTIONS)
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

		KVSocket a_socket;
		a_socket.socket_id = new_socket;
		a_socket.type = "client";
		a_socket.name = "";
		socket_vector.push_back(a_socket);

		// Keep track of the last update to the sockets
		gettimeofday(&sockets_last_modified, NULL);

		break;
	}
}

void KVServer::handleExistingConnections()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Iterate over all clients
	for (int i = 0; i < MAX_CONNECTIONS; i++) 
	{
		if (!FD_ISSET(sockets[i], &socket_descriptors)) continue;

		// Clear out the buffer
		memset(&buffer[0], 0, BUFFER_SIZE);

		// Read the incoming message into the buffer
		int message_size = read(sockets[i], buffer, INCOMING_MESSAGE_SIZE);

		// Handle a closed connection
		if (message_size == 0)
		{
			// Report the disconnection
			getpeername(sockets[i], (struct sockaddr*)&client_address, &client_address_length);
			cout << "Connection closed: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

			// Close and free the socket
			//queueSocketToClose(sockets[i]);
			closeSocket(sockets[i]);
		}
		else
		{
			/*
				If we had a single thread, we'd handle requests here
				So maybe we'll handle async requests here?

				Otherwise, if we're using MT, we need to handle all
				incoming and outgoing communications on the threads.

				Not real sure how we'll accomplish that yet.
			*/
		}
	}
}

/**
 * Handle closing sockets
 */

void KVServer::queueSocketToClose(int socket_id)
{
	sockets_to_close.push_back(socket_id);
}

void KVServer::closeSocket(int socket)
{
	// Remove the socket from the vector
	vector<KVSocket>::iterator iter;
	for (iter = socket_vector.begin(); iter != socket_vector.end(); )
	{
		if (iter->socket_id == socket)
			iter = socket_vector.erase(iter);
		else
			++iter;
	}

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);

	// Remove from the int array
	for (int i = 0; i < MAX_CONNECTIONS; i++)
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

