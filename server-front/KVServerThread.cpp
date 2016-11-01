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
	if (DEBUG_MODE)
		printf("Socket number: %d\n", socket_fd);

	// Keep track of performance details
	perf_cp = new vector<string>();
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
	// Some common, shared info for each session
	auto thread_id = this_thread::get_id();
	stringstream ss;
	ss << thread_id;
	string threadId = ss.str();
	
	while (true) 
	{
		if (DEBUG_MODE)
			cout << "... Listening for activity on thread..." << endl;

		// Construct the FD to listen to
		struct pollfd fds[1];
		fds[0].fd     = socket_fd;
		fds[0].events = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLWRBAND;

		// Wait for activity
        int activity = poll(fds, 1, 0);

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			perror("Error: select failed");
			exit(1);
		}
		else
		{
			performance_checkpoint(perf_cp, "bmessage");
			bool is_socket_open = KVServer::handleMessage(socket_fd);
			performance_checkpoint(perf_cp, "emessage");
			if (!is_socket_open) {
				print_checkpoints_to_file(perf_cp, "st_" + threadId);
				return;
			}
		}
	}
}
