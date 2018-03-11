This is a chat project that allows separate window chat between client(c) and
server(python 3.6) programs. Client and server _take_turns_ responding. Client
responds first. Client can end conversation by sending server "\quit", then
server must be closed by SIGINT. Server can end conversation by sending client
"\quit", at which point client program closes.
------------------------------------------------------------------------------
Note: chatserve.py was created and tested with Python 3.6.1

------------------------------------------------------------------------------
Order for initiating chat session:
	1. Build chatclient
	2. Start chatserve.py in one window
	3. Start chatclient in another window

1. To build chatclient program:
$ make all
(ignore warnings)

2. Unless specified, chatserve.py runs with a default hostname and portnum

	2.1 To run chatserve.py on default hostname "localhost" and portnum 12300:
	$ python3 chatserve.py

	2.2 To run chatserve.py with chosen hostname and portnum:
	$ python3 chatserve.py --hostname yourhostname --port yourportnum

3. Currently, to run chatclient program hostname and portnum must be specified
$ ./chatclient yourhostname yourportnum
