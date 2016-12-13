# KV511
A key-value store implemented with a client-server model.

# Requirements:
- Make
- G++ 4.9

# About the JSON Parser
JSON parser is a C++ library provided here: https://github.com/nlohmann/json

## Design
###Client Design
The client is a multi-threaded program running separately from the server. It creates a connection with the server and then issues a sequence of sessions - each consisting of a series of get or post requests. The client receives responses from the server, but at this point does carry out any action with the response. The client threads use blocking network IO in their connection with the server.
Upon starting up, clients read in a configuration file containing the connection info needed to connect to the server as well as the number of threads to spawn to handle requests. The existing client implementation includes functionality to call a script on the client machine, primarily to generate sessions, but could be extended to allow the client to be plugged into an existing application to enable network key/value storage.

The client implements a two level time-out polciy - first reissuing the request then terminating the connection.

###Server Front-End Design
The server front end runs in two different modes: multithreaded with blocking network IO and single-threaded with asynchronous network IO. The multithreaded mode uses pthreads to achieve our goals. The server front end provides a simple request API handling post and get requests and responding appropriately. At this point, the server data store is an in-memory hashmap.

Upon initializing, the server front-end begins listening on port 56789. If running in multithreaded mode, the main thread creates a new server thread to handle that connection. A MAX_SESSIONS variable defines the maximum number of allowable simultaneous connections if the number of requesting threads exceeds this value, the server will issue a not accepting connections error but continue handling existing connections. In single-thread mode, the main thread handles the connection itself.

The server front end utilizes a quorum-based replication scheme where all writes are replicated to at least W nodes, while all reads are retrieved from at least R nodes with the highest version number having priority. R+W > N, W > N/2.

###Server Back-End Design


####Data Store
The front-end provides an in-memory cache of the key-value pairings with a least recently used replacement policy. This is currently implemented with a C++ unordered map and a linked list. The key/value pairs are strings.

The back-end provides persistent storage of the key-value pairings and the version number. The storage medium is a space separated file where each line is a separate entry and each line takes the form key version value. We chose to do this because the size of the file will grow linearly with respect to the number of individual keys used. Using a single file also makes finding the current stored value trivial rather than needing to search through multiple files. The alternative of keeping a separate file per key takes more space without providing a seek improvement while the alternative of keeping a separate file per version increases both seek speed space required.

###Communication Protocol
Communication between client and server takes place using json messages. The JSON includes the request type, key, (optional) value, and (for responses) code.

## Compiling
Build the client and server

~~~
cd KV511/client-front
make
cd KV511/server-front
make
cd KV511/server-back
make
~~~
## Running
Start the server front

~~~
cd KV511/server-front
./server.o 0 //for threaded version
./server.o 1 //for asynch version
~~~

 Start the server back on each host
 ~~~
cd KV511/server-back
./server-back.o
~~~


Quitting the server

~~~
`q`

By entering "q" and hitting enter, the user can quit the server program.
~~~

Start the client

~~~
cd KV511/client-front
./client.o <config-file> // config file specifies ip address and port to connect to, as well as spec type and thread count
~~~

Quitting the client

~~~
`q`

By entering "q" and hitting enter, the user can quit the client program.
~~~

Client Config Specification:

~~~
Pull in data by the expected input
<ip address/localhost> <port number> <specification type> <number of threads>

Right now, the specification type does not actually change the output of the program. However, the number of threads can determine whether the program acts as "type 1" or "type 2".
~~~
