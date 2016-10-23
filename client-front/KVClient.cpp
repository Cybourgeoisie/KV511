/**
 * Distributed Key-Value Store client class
 */

#include "KVClient.hpp"

using namespace std;

/**
 * Public Methods
 */

KVClient::KVClient() {}

void KVClient::start(string address, int port, int spec_type, int thread_count)
{
	// Clear the screen to boot
	KVCommon::clearScreen();
	// Construct the connection details
	KVConnectionDetails details;
	details.address = address;
	details.port = port;

	// Set up the threads, depending on our spec type
	spawnThreads(&details, thread_count, spec_type);

	// Now open up the menu
	runProgram();
}

/**
 * spec_type is long to avoid a warning from the g++ compiler about the (void *) conversion
 */
void * KVClient::spawnThreads(KVConnectionDetails * details, int thread_count, long spec_type)
{
	for (int i = 0; i < thread_count; i++)
	{
		// Spawn the new thread
		pthread_t node_thread;

		if (pthread_create(&node_thread, NULL, &KVClient::startClientThread, (void *) details) != 0)
		{
			perror("Error: could not spawn peer listeners");
			exit(1);
		}

		// Keep track of the threads
		threads.push_back(node_thread);
	}
}

void * KVClient::startClientThread(void * arg)
{
	KVConnectionDetails * details;
	details = (KVConnectionDetails *) arg;

	KVClientThread * thread = new KVClientThread(details);
	thread->sendRequests();
	pthread_exit(NULL);
}

void KVClient::runProgram()
{
	bool b_program_active = true;
	while (b_program_active)
	{
		b_program_active = runUI();
	}
}

bool KVClient::runUI()
{
	char option = showMenu();

	switch (option)
	{
		case 'v':
			KVCommon::clearScreen();
			viewThreads();
			break;
		case 'q':
			break;
		default:
			cout << endl << "That selection is unrecognized. Please try again." << endl;
			break;
	}

	return (option != 'q');
}

char KVClient::showMenu()
{
	cout << "Welcome to the KV Network. What would you like to do?" << endl;
	cout << "Please select an item below by entering the corresponding character." << endl << endl;
	cout << "\t (v) View Node Progress" << endl;
	cout << "\t (q) Quit" << endl << endl;

	// Take the client's order
	string option;
	getline(cin, option, '\n');

	// Cast to a char
	return option[0];
}

void KVClient::viewThreads()
{
	cout << endl;
	cout << "Number of created threads: " << threads.size() << endl << endl;
}
