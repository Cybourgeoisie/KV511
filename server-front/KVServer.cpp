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

// Be aware of the global var provided by server.cpp
extern bool ASYNC_MODE;

KVServer::KVServer()
{
	// Define the port number to listen through
	PORT_NUMBER = 56789;

	// Define server limits
	MAX_SESSIONS = 512;
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Keep track of the client sockets
	sockets = new int[MAX_SESSIONS];

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];
}

void KVServer::start(bool use_async)
{
	// Determine if we want to use MT
	ASYNC_MODE = use_async;

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

void KVServer::listenForActivity()
{
	KVCommon::clearScreen();

	while (true) 
	{
		cout << "... Listening for new connections on main thread..." << endl;

		// Ready the socket descriptors
		this->resetSocketDescriptors();

		// Determine if we're listening to all ports or just the primary
		int activity;
		if (ASYNC_MODE)
		{

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

		}
		else
		{
			// In MT mode, the primary only handles new connection requests
			this->handleNewConnectionRequest();
		}
	}
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

