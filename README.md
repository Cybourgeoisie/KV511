# KV511
A key-value store implemented with a client-server model.

# Requirements:
- Make
- G++ 4.9

# About the JSON Parser
JSON parser is a C++ library provided here: https://github.com/nlohmann/json

## Design
###Client Design
The client is a multi-threaded program running separatly from the server. It creates a connection with the server and then issues a sequence of sessions - each consisting of a series of get or post requests. The client receives responses from the server, but at this point does carry out any action with the response. The client threads use blocking network IO in their connection with the server.
Upon starting up, clients read in a configuration file containing the connection info needed to connect to the server as well as the number of threads to spawn to handle requests. The existing client implementation includes functionality to call a script on the client machine, primarily to generate sessions, but could be extended to allow the client to be plugged into an existing application to enable network key/value storage.

###Server Design
####Data Store
At this point, the server provides an in-memory cache of the key-value pairings. This is currently implemented with a C++ unordered map. The key/value pairs are strings.
###Communication Protocol
Communication between client and server takes place using json messages. The JSON includes the request type, key, (optional) value, and (for responses) code.

## Compiling
Build the client and server

~~~
cd KV511/client-front
make
cd KV511/server-front
make
~~~
## Running
Start the server

~~~
cd KV511/server-front
./server.o 0 //for threaded version
./server.o 1 //for asynch version
~~~

Start the client

~~~
cd KV511/client-front
./client.o <config-file> // config file specifies ip address and port to connect to, as well as spec type and thread count
~~~

Client Config Specification:

~~~
Pull in data by the expected input
<ip address/localhost> <port number> <specification type> <number of threads>
Specification types are as follows:
0 - purely debugging, single threaded, send a few sessions
1 - "type 1" defined on page 4; single thread, ~100 sessions
2 - "type 2" defined on page 4; <Method still TBD>
~~~
