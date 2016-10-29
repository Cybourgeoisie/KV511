/**
 * Distributed Key-Value Store client thread class
 */

#include "KVClientThread.hpp"
#include "../libs/KVApi.cpp"

using namespace std;
using namespace nlohmann;

/**
 * Public Methods
 */

KVClientThread::KVClientThread(KVConnectionDetails * conn_details)
{
	details = conn_details;
}

void KVClientThread::sendRequests()
{
	// Some common, shared info for each session
	auto thread_id = this_thread::get_id();
	stringstream ss;
	ss << thread_id;
	string threadId = ss.str();

	// Generate the filename
	string fileName = threadId;
	string sessionTxt = "_Session";
	fileName.append(sessionTxt);

	// Generate the session creation command
	string command = "python ../libs/jsonGenReq.py ";
	command.append(to_string(SESSION_LENGTH));
	command.append(" ");
	command.append(fileName);
	cout << "Generated filename: " << command << endl;

	KVApi * api = new KVApi(details);
	for (int i = 0; i < 3; i++)
	{
		// Open the connection, send the session's requests
		if (api->open())
		{
			cout << "Connection made." << endl;

			// Run the Python script
			system(command.c_str());

			// Pull back in the created JSON
			json inputJson;
			inputJson = json::parse(get_file_contents(fileName.c_str()));

			// Send all of the messages
			for (json::iterator it = inputJson.begin(); it != inputJson.end(); ++it)
			{
				// Send the message
				string type;
				if ((*it)["type"].is_string())
					type = (*it)["type"].get<string>();

				// Get the key
				string key;
				if ((*it)["key"].is_string())
					key = (*it)["key"].get<string>();
				else if ((*it)["key"].is_number())
					key = to_string((*it)["key"].get<int>());
				else
					continue; // Can't handle this case

				// Submit the GET or POST request
				if (strcmp(type.c_str(),"GET") == 0)
				{
					api->get(key);
				}
				else if (strcmp(type.c_str(),"POST") == 0)
				{
					// Get the value
					string value;
					if ((*it)["value"].is_string())
						value = (*it)["value"].get<string>();
					else if ((*it)["value"].is_number())
						value = to_string((*it)["value"].get<int>());
					else
						continue; // Can't handle this case

					api->post(key, value);
				}
			}

			api->close();
		}

		// Need to sleep at least a little bit, otherwise the Async server
		// could starve threads (currently accessing sockets linearly)
		usleep(10000); // 10 ms
		//sleep(1);
	}

	cout << "-- Thread ID " << threadId << " finished --" << endl;
}
