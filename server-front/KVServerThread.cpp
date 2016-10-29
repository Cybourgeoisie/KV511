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
			bool is_socket_open = KVServer::handleMessage(socket_fd);
			if (!is_socket_open)
				return;
		}
	}
}
