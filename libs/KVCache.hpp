#ifndef KVCACHE_H
#define KVCACHE_H

using namespace std;

// Mutexes
static pthread_mutex_t mutex_cache = PTHREAD_MUTEX_INITIALIZER;

class KVCache
{
	private:

		// The cache
		static unordered_map<string, string> hashtable;

	public:

		KVCache();

		// API
		static bool get_value(const string&, string&);
		static bool post_value(const string, string);
		static void print_contents();

};

#endif