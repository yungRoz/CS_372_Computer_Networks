###########################################################
# Author: Romano Garza
# Date: 02/11/18
# Description: Client program for carying out FTP w/ server
###########################################################
from sys import argv
from socket import *


###########################################################
# Descript: used to parse command line arguments and create
# a dictionary from them
###########################################################
# Source: https://gist.github.com/dideler/2395703


def getopts(argv):
    opts = {}  # Empty dictionary to store key-value pairs.
    opts['serverHost'] = argv[1] + '.engr.oregonstate.edu'
    opts['serverPort'] = int(argv[2])
    opts['command'] = argv[3]
    if opts['command'] == '-l':
        opts['dataPort'] = int(argv[4])
        opts['filename'] = None
    else:
        opts['dataPort'] = int(argv[5])
        opts['filename'] = argv[4]
    return opts


###########################################################
# Descript: validates arguments
###########################################################
def val(myargs):
    validHosts = ['flip1.engr.oregonstate.edu',
                  'flip2.engr.oregonstate.edu',
                  'flip3.engr.oregonstate.edu']
    validCommands = ['-l', '-g']

    if myargs['serverHost'] not in validHosts:
        print("ERROR: invalid Server Host Name")
        exit(1)
    if myargs['serverPort'] not in range(3000, 65535):
        print("ERROR: invalid Server Port Number")
        exit(1)
    if myargs['command'] not in validCommands:
        print("ERROR: invalid command passed")
        exit(1)


###########################################################
# Description: Checks the passed string for the quit phrase
###########################################################
def continueConvo(string):
    if quitPhrase in string:
        return False
    else:
        return True

###########################################################
# Description: The messenger class uses a port number and
# host name to set up a socket for sending and receiving
# message with a client
###########################################################


class Messenger:
    # stores the port number and host name upon init
    def __init__(self, args):
        self.pnum = args['serverPort']
        self.hname = args['serverHost']

    # set up the socket
    def setUpSocket(self):
        self.serverSocket = socket(AF_INET, SOCK_STREAM)

    # connect to server
    def connectToServer(self):
        self.serverSocket.connect((self.hname, self.pnum))
    # listen for server

    def listenForServer(self):
        self.serverSocket.bind(('', self.pnum))
        self.serverSocket.listen(1)

    # wait for connection and incoming messages
    def waitAndReceive(self):
        self.connectionSocket, self.addr = self.serverSocket.accept()
        message = self.connectionSocket.recv(500)
        return message.decode()
    # variation of wait and receive
    # that also gets incoming amount

    def waitAndRecAmnt(self):
        self.connectionSocket, self.addr = self.serverSocket.accept()
        amnt = self.connectionSocket.recv(10)
        amnt = amnt.decode()
        amnt = int(amnt.split('\x00')[0])
        self.amnt = amnt

    # gets the incoming message relating to the incoming ammount
    # stores this as an int in self.amnt, the 1000 amount is
    # a flag that means the client is closing the connection
    def getIncomingAmount(self):
        amnt = self.connectionSocket.recv(10)
        amnt = amnt.decode()
        amnt = int(amnt.split('\x00')[0])
        self.amnt = amnt

        if self.amnt == 1000:
            self.sendMessage("Chatserver> Peace out\n")
            print("Client left conversation")
            return 0
        return 1

    # sends a message to the client
    def sendMessage(self, message):
        self.connectionSocket.send(message.encode("utf-8"))

    # gets clients full message
    def getMessage(self):
        read = 0
        fullMessage = ""
        while len(fullMessage) < self.amnt:
            portion = self.connectionSocket.recv(10)
            portionDecoded = portion.decode()
            fullMessage += portionDecoded
            read += len(portionDecoded)
        # remove new line characters
        fullMessage = fullMessage.replace("\n", "")
        return fullMessage

    # closes connection
    def close(self):
        self.connectionSocket.close()

    def getClientHostName(self):
        import socket
        ipAddress = socket.gethostbyname(socket.gethostname)
        response = socket.gethostbyaddr(ipAddress)
        hostname = response[0][:5]
        return hostname


if __name__ == '__main__':
    from sys import argv
    # check for too few or too many arguments passed
    if len(argv) not in (5, 6):
        print("ERROR: incorrect number of arguments passed")
        exit(1)

    myargs = getopts(argv)
    val(myargs)

    # set up messenger device for outer client connection
    client = Messenger(myargs)
    clientHostName = client.getClientGetHostName()
    # set up socket
    client.setUpSocket()
    # connect
    client.connectToServer()

    # send client hostname messages
    client.sendMessage(clientHostName)
    # get incoming amount for confirmation message
    check = client.getIncomingAmount()
    # send message that amount was received
    client.sendMessage('received amount')
    # get message
    allClear = client.getMessage()  # should be "Got" message from server

    # send port number message
    client.sendMessage(str(myargs['serverPort']))
    # get incoming amount for confirmation message
    check = client.getIncomingAmount()
    # send message that amount was received
    client.sendMessage('received amount')
    # get message
    allClear = client.getMessage()  # should be "Got" message from server

    # send command
    client.sendMessage(myargs['command'])
    # get incoming amount for confrimation
    check = client.getIncomingAmount()
    # send message that amount was received
    client.sendMessage('received amount')
    # get message - note - should be "Got"
    # if not, an invalid command was sent
    allClear = client.getMessage()
    # print error message if
    # error message was received
    if "Got" not in allClear:
        print(allClear)
        exit(1)

    # set up messenger device for inner data connection
    dataSocket = Messenger(myargs)
    # set up socket
    dataSocket.setUpSocket()
    # listen for server
    dataSocket.listenForServer()
    # wait for connect and then receive the amount
    check = dataSocket.waitAndRecAmnt()
    # send back confirmation
    dataSocket.sendMessage('received amount')
    dataSocket.getMessage()

    if myargs['command'] is '-l':
        dataSocket.sendMessage('send directories')
        dataSocket.getAmount()
        datasocket.sendMessage('received amount')
        # get message containing directories
        message = dataSocket.getMessage()
        print(message)
        #messages = splitUpSpaces(message)
    else:
        dataSocket.sendMessage(myargs['filename'])
        dataSocket.getAmount()
        datasocket.sendMessage('received amount')
        # get message containing directories
        message = dataSocket.getMessage()
        print(message)
    print("The server is ready to receive")
    dataSocket.close()
    client.close()
