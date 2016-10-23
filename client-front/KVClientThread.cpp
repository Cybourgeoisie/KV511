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
	//makeConnection();
	printf("Connecting to %s\t%d\n", details->address.c_str(), details->port);

	long i = 0;
	while (1)
	{
		i += 1;
	}
}


/** 
 * Private Methods
 */

/**
 * Make a connection
 */
void KVClientThread::makeConnection(string host, int port)
{
	struct sockaddr_in server_address;
	struct hostent * server;

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		cout << "Error: could not open socket" << endl;
		return;
	}

	// Find the server host
	server = gethostbyname(host.c_str());
	if (server == NULL)
	{
		cout << "Error: could not find the host" << endl;
		return;
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
		return;
	}

	cout << "Connected to server on port " << port << endl;
}
