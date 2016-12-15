/**
 * Distributed Key-Value Store server class
 */

#include "KVServerBack.hpp"
#include "../libs/KVCommon.cpp"

/**
 * Public Methods
 */

vector<int> KVServerBack::sockets_to_close;

int KVServerBack::BUFFER_SIZE = 513; // Size given in bytes
int KVServerBack::INCOMING_MESSAGE_SIZE = KVServerBack::BUFFER_SIZE - 1;
string KVServerBack::STORAGE_FILE = "storage.txt"; // Default storage file name

KVServerBack::KVServerBack()
{
	// Define the port number to listen through
	PORT_NUMBER = 56790; // DEFAULT - overridden in ::start(int)

	// Define server limits
	MAX_SESSIONS = 512;

	// Keep track of the client sockets
	sockets = new int[MAX_SESSIONS];

	// Keep track of performance details
	perf_cp = new vector<string>();
}

void KVServerBack::start(int port)
{
	// Set the port
	PORT_NUMBER = port;

	// Update the filename
	ostringstream oss;
	oss << "storage_" << port << ".txt";
	STORAGE_FILE = oss.str();

	// Open the socket and listen for connections
	initialize();
	openSocket();

	// Now listen for updates
	listenForActivity();
}

/**
 * Private Methods
 */

void KVServerBack::initialize()
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

void KVServerBack::openSocket()
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

    /* ASYNC
	if (ioctl(primary_socket, FIONBIO, (char *)&opt) < 0)
	{
		perror("Error: ioctl() failed to make primary socket non-blocking");
		close(primary_socket);
		exit(-1);
	}
	*/

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

int KVServerBack::bindSocket(int socket)
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

void KVServerBack::listenForActivity()
{
	if (DEBUG_MODE)
		KVCommon::clearScreen();

	while (true) 
	{
		// Ready the socket descriptors
		this->resetSocketDescriptors();

		// Wait for activity - all sockets
		int activity;
		activity = select(max_connection + 1, &socket_descriptors, NULL, NULL, NULL);

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
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
		else
		{
			// Anything on the primary socket is a new connection
			if (FD_ISSET(primary_socket, &socket_descriptors))
			{
				this->handleNewConnectionRequest();
			}

			// Perform any open activities on all other clients
			this->handleExistingConnections();
		}
	}
}

void KVServerBack::sendMessageToSocket(string request, int socket) {
    //write the message to the client socket
    if (write(socket, request.c_str(), request.length()) < 0){
        perror("Error: could not send message to client");
        exit(1);
    }
}

string KVServerBack::createResponseJson(string type, string key, string value, int version, int code) {
    json response;
    response["type"] = type;
    response["key"] = key;
    response["value"] = value;
    response["version"] = version;
    response["code"] = code;
    return response.dump();
}


/**
 * Handle Connection Activity
 */

void KVServerBack::resetSocketDescriptors()
{
	// Determine if we have any sockets to close
	vector<int> copy_of_sockets_to_close;

	// For (1) fair comparison with MT, and (2) to keep code cleaner
	copy_of_sockets_to_close = KVServerBack::sockets_to_close; // Make a copy of the data
	KVServerBack::sockets_to_close.clear(); // Empty the sockets to close

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

void KVServerBack::handleNewConnectionRequest()
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
		break;
	}
}

void KVServerBack::handleExistingConnections()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Iterate over all clients
	for (int i = 0; i < MAX_SESSIONS; i++) 
	{
		if (!FD_ISSET(sockets[i], &socket_descriptors)) continue;

		performance_checkpoint(perf_cp, "bmessage");
		KVServerBack::handleMessage(sockets[i]);
		performance_checkpoint(perf_cp, "emessage");
	}
}

bool KVServerBack::handleMessage(int socket_fd)
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

		KVServerBack::sockets_to_close.push_back(socket_fd);

		return false;
	}
	else
	{
		//parse message and execute accordingly
		json request = json::parse(buffer);
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

			KVResult_t result = KVServerBack::get(key);
			response = KVServerBack::createResponseJson("GET", result.key, result.value, result.version, result.err);
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

			int version;
			if (request["version"].is_string())
				version = stoi(request["version"].get<string>());
			else if (request["version"].is_number())
				version = request["version"].get<int>();

			KVResult_t result = KVServerBack::post(key, value, version);
			response = KVServerBack::createResponseJson("POST", result.key, result.value, result.version, result.err);
		}

		// Return the result to the client
		KVServerBack::sendMessageToSocket(response, socket_fd);
	}

	return true;
}

KVResult_t KVServerBack::get(string key)
{
	KVResult_t result;
	result.key = key;
	result.err = 404; // Default: Not Found
	result.version = 0;
	result.value = "";

	// Return key, version, value
	ifstream is;

	// Prepare the key for search	
	ostringstream os;
	os << key << " ";
	string s_key = os.str();

	// Open the file to search
	is.open(STORAGE_FILE, ios::binary);

	// Find the line
	string line;
	while (getline(is, line))
	{
		// See if the keys match
		if (line.find(s_key) == 0)
		{
			// Get the version number and value
			smatch m;
			regex e("^([0-9]{1,}) ([0-9]{1,}) (.*)$");
			if (regex_search(line, m, e))
			{
				// Get the version number
				int version = stoi(m.str(2));

				// Get the value
				string value = m.str(3);

				result.err = 200; // Found
				result.version = version;
				result.value = value;

				// Close the file stream and return the value
				is.close();
				return result;
			}
		}
	}

	is.close();
	return result;
}

KVResult_t KVServerBack::post(string key, string value, int version)
{
	KVResult_t result;
	result.value = value;
	result.key = key;
	result.err = 200;
	result.version = version;

	// Well, C++ requires writing a new file every time we need to update a part of it, so...
	// See if the key is already in storage
	if (KVServerBack::exists(key))
	{
		// Prepare the new filename
		ostringstream os_name;
		os_name << STORAGE_FILE << ".tmp";
		string new_filename = os_name.str();

		// Prepare the key for search
		ostringstream os_key;
		os_key << key << " ";
		string s_key = os_key.str();

		// Write to the file
		ofstream os;
		os.open(new_filename, ios::app);

		// Read the file
		ifstream is;
		is.open(STORAGE_FILE, ios::binary);

		// Copy the file line by line
		string line;
		while (getline(is, line))
		{
			// See if the keys match
			if (line.find(s_key) == 0)
			{
				// Get the version number and value
				smatch m;
				regex e("^([0-9]{1,}) ([0-9]{1,}) (.*)$");
				if (regex_search(line, m, e))
				{
					// If the (update) version provided is invalid, get from the file
					if (version < 0)
						version = stoi(m.str(2));

					// Increment to the next version
					version += 1;

					// Update the result
					result.version = version;

					// Write the new value and version number
					os << key << " " << version << " " << value << endl;
				}
			}
			else
			{
				// Just write the line
				os << line << endl;
			}
		}

		// When we're done, close and move
		os.close();
		is.close();
		rename(new_filename.c_str(), STORAGE_FILE.c_str());
	}
	else
	{
		// Otherwise append
		ofstream os;
		os.open(STORAGE_FILE, ios::app);
		os << key << " 1 " << value << endl; // Version 1

		result.version = 1;
	}

	return result;
}

bool KVServerBack::exists(string key)
{
	// Return key, version, value
	ifstream is;

	// Prepare the key for search	
	ostringstream os;
	os << key << " ";
	string s_key = os.str();

	// Open the file to search
	is.open(STORAGE_FILE, ios::binary);

	// Find the line
	string line;
	while (getline(is, line))
	{
		// See if the keys match
		if (line.find(s_key) == 0)
		{
			is.close();
			return true;
		}
	}

	is.close();
	return false;
}


/**
 * Handle closing sockets
 */

void KVServerBack::closeSocket(int socket)
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
