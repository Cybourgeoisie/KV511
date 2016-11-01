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


std::string get_file_contents(const char *filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

static __inline__ void performance_checkpoint(vector<string> * v, string ref)
{
    struct timespec t;
	clock_gettime(KV511_CLOCK_TIMING, &t);
	uint64_t t_int = 1000000000L * (t.tv_sec) + t.tv_nsec;
	v->push_back(ref + " " + to_string(t_int));
}

void print_checkpoints_to_file(vector<string> * v, string filename)
{
	ostringstream oss;

	if (!v->empty())
	{
		copy(v->begin(), v->end()-1, ostream_iterator<string>(oss, "\n"));
		oss << v->back();
	}

	ofstream checkout_file;
	checkout_file.open("../checkpoints/" + filename + ".txt");
	checkout_file << oss.str() << endl;
	checkout_file.close();
}
