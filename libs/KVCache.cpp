/**
 * Distributed Key-Value Store server's cache class
 */

#include "KVCache.hpp"

/**
 * Public Methods
 */

unordered_map<string, string> KVCache::hashtable;

// Be aware of the global var provided by server.cpp
extern bool ASYNC_MODE;

KVCache::KVCache() { }

bool KVCache::get_value(const string& key, string& value)
{
	if (ASYNC_MODE)
	{
		// Async mode should avoid blocking I/O
		auto t = hashtable.find(key);

		if (t == hashtable.end())
		{
			return false;
		}

		value = t->second;
	}
	else
	{
		// Multithreading mode requires blocking I/O
		pthread_mutex_lock(&mutex_cache);
		auto t = hashtable.find(key);

		if (t == hashtable.end())
		{
			pthread_mutex_unlock(&mutex_cache);
			return false;
		}

		value = t->second;
		pthread_mutex_unlock(&mutex_cache);
	}

	return true;
}

bool KVCache::post_value(const string key, string value)
{
	if (ASYNC_MODE)
	{
		// Async - Non-blocking post
		hashtable[key] = value;
	}
	else
	{
		// MT - Blocking post
		pthread_mutex_lock(&mutex_cache);
		hashtable[key] = value;
		pthread_mutex_unlock(&mutex_cache);
	}

	return true;
}

// For testing
void KVCache::print_contents()
{
	if (ASYNC_MODE)
	{
		cout << "hashtable contains:";
		for ( auto it = hashtable.begin(); it != hashtable.end(); ++it )
			cout << " " << it->first << ":" << it->second;
		cout << endl;		
	}
	else
	{
		pthread_mutex_lock(&mutex_cache);
		cout << "hashtable contains:";
		for ( auto it = hashtable.begin(); it != hashtable.end(); ++it )
			cout << " " << it->first << ":" << it->second;
		cout << endl;
		pthread_mutex_unlock(&mutex_cache);		
	}
}