#ifndef KVCOMMON_H
#define KVCOMMON_H

// Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <map>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <thread>
#include <cerrno>

// Directory Management
#include <dirent.h>

// Network Includes
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>


// Multithreading
#include <pthread.h>

// JSON parsing support
#include "json.hpp"

using namespace std;

typedef struct {
	string address;
	int port;
} KVConnectionDetails;

class KVCommon
{
	public:
		KVCommon();
		static bool fileExists(string);
		static vector<string> parseRequest(string);
		static vector<string> parseAddress(string);
		static vector<string> splitString(string, char);
		static string trimWhitespace(string);
		static void clearScreen();
};
std::string get_file_contents(const char *);

#define SESSION_LENGTH 10
#define DEBUG_MODE true

#endif