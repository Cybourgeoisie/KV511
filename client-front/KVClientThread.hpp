#ifndef KVCLIENTTHREAD_H
#define KVCLIENTTHREAD_H

#include "../libs/KVApi.hpp"

using namespace std;

class KVClientThread
{
	private:

		// Keep track of the server connection & details
		KVConnectionDetails * details;

		// Keep track of performance details
		vector<string> * perf_cp;

public:
		KVClientThread(KVConnectionDetails *);
		void sendRequests();

};

#endif