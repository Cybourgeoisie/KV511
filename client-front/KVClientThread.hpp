#ifndef KVCLIENTTHREAD_H
#define KVCLIENTTHREAD_H

#include "../libs/KVApi.hpp"

using namespace std;

class KVClientThread
{
	private:

		// Keep track of the server connection & details
		KVConnectionDetails * details;

public:
		KVClientThread(KVConnectionDetails *);
		void sendRequests();

};

#endif