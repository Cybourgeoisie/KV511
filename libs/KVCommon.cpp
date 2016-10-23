/**
 * Distributed Key-Value Store Common Tools
 */

#include "KVCommon.hpp"

KVCommon::KVCommon() {}

bool KVCommon::fileExists(string filename)
{
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

vector<string> KVCommon::parseRequest(string request)
{
	return KVCommon::splitString(request, '\n');
}

vector<string> KVCommon::parseAddress(string request)
{
	return KVCommon::splitString(request, ':');
}

vector<string> KVCommon::splitString(string request, char delimeter)
{
	// Remove spaces from the request
	stringstream ss(request);
	vector<string> request_parsed;

	if (request.length() > 0)
	{
		string line;
		while (getline(ss, line, delimeter))
		{
			request_parsed.push_back(line);
		}
	}

	return request_parsed;
}

string KVCommon::trimWhitespace(string line)
{
	// Trim trailing whitespace
	line.erase(line.find_last_not_of(" \n\r\t")+1);

	// Trim leading whitespace
	if (line.length() > 0)
	{
		int first_non_whitespace = line.find_first_not_of(" \n\r\t");
		if (first_non_whitespace > 0)
		{
			line = line.substr(first_non_whitespace, line.length() - first_non_whitespace);
		}
	}

	return line;
}

void KVCommon::clearScreen()
{
	cout << "\033[2J\033[1;1H";
}