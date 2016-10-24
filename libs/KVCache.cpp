/**
 * Distributed Key-Value Store server's cache class
 */

#include "KVCache.hpp"

/**
 * Public Methods
 */

unordered_map<string, string> KVCache::hashtable;

KVCache::KVCache() { }

bool KVCache::get_value(const string& key, string& value)
{
	pthread_mutex_lock(&mutex_cache);
	auto t = hashtable.find(key);

	if (t == hashtable.end())
	{
		pthread_mutex_unlock(&mutex_cache);
		return false;		
	}

	value = t->second;

	pthread_mutex_unlock(&mutex_cache);
	return true;
}

bool KVCache::post_value(const string key, string value) {
	pthread_mutex_lock(&mutex_cache);
	hashtable[key] =  value;
	pthread_mutex_unlock(&mutex_cache);
	return true;
}

void KVCache::print_contents()
{
	pthread_mutex_lock(&mutex_cache);
	cout << "hashtable contains:";
	for ( auto it = hashtable.begin(); it != hashtable.end(); ++it )
		cout << " " << it->first << ":" << it->second;
	cout << endl;
	pthread_mutex_unlock(&mutex_cache);
}