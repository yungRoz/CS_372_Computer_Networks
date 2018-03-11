This is a project that implements FTP between client(python 3.6) and
server(c) programs over socket connections. Once connected, client can request
server directory contents or download a '.txt' file from the server.
------------------------------------------------------------------------------
Note: chatserve.py was created and tested with Python 3.6.1 on Oregon States
flip servers 
------------------------------------------------------------------------------
Order for initiating FTP:
	1. Build ftserver
	2. Start ftserver in one terminal window
	3. Start ftclient.py in another terminal window

1. To build ftserver program:
$ make all
(ignore warnings)

2. ftserver must be run with server port number specified:
$ ./ftserver <SERVER_PORT>

3. ftclient.py must be run with server port number, server hostname,
	 command, optional filename, and data port number

	There are two commands that may be issued '-l' for directory contents or
	'-g' for getting a file
	3.1 To run ftclient.py for directory contents:
	$ python3 ftclient.py <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>

	3.2 To run ftclient.py for file transfer filename option must be included:
	$ python3 ftclient.py <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>
