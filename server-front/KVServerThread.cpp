/**
 * Distributed Key-Value Store server class
 */

#include "KVServerThread.hpp"

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
				cout << buffer << endl;
			}
			else
			{
				cout << "Closed the connection" << endl;
				
				// Close and free the socket
				close(socket_fd);

				// Leave!
				return;
			}
		}
	}
}
