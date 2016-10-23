#include <iostream>
#include "KVClient.cpp"

using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "KV Client - startup requested" << endl;

	// Get any custom arguments
	string config_filename;
	if (argc == 2)
	{
		config_filename = argv[1];
	}
	else
	{
		cerr << "Required arguments: <configuration filename>" << endl;
		return -1;
	}

	// Validate that the configuration file exists
	if (!KVCommon::fileExists(config_filename))
	{
		cerr << "Config file does not exist" << endl;
		return -1;
	}

	/**
	 * Pull in data by the expected input
	 * <ip address/localhost> <port number> <specification type> <number of threads>
	 *
	 * Specification types are as follows:
	 * 0 - purely debugging, single threaded, send a few sessions
	 * 1 - "type 1" defined on page 4; single thread, ~100 sessions
	 * 2 - "type 2" defined on page 4; <Method still TBD>
	 *
	 */

	// The necessary information
	string address;
	int port, spec_type, thread_count;

	 // Read in the configuration
	ifstream inputFile(config_filename);
	string line;

	// Assume that we have ONE line in the above format
	if (getline(inputFile, line))
	{
		// Read in the line
		istringstream ss(line);

		// Feed into the variables
		ss >> address >> port >> spec_type >> thread_count;
	}

	// Start up the client server
	KVClient client;
	client.start(address, port, spec_type, thread_count);

	return 0;
}