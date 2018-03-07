###########################################################
## Author: Romano Garza
## Date: 02/11/18
## Description: Server program for chating with a client
###########################################################
from sys import argv
from socket import *

hostname = "localhost" #default hostname
portnum = 12300 #default port number
quitPhrase = "\quit"

###########################################################
## Descript: used to parse command line arguments and create
## a dictionary from them
###########################################################
# Source: https://gist.github.com/dideler/2395703

def getopts(argv):
    opts = {}  # Empty dictionary to store key-value pairs.
    while argv:  # While there are arguments left to parse...
        if argv[0][0] == '-':  # Found a "-name value" pair.
            opts[argv[0]] = argv[1]  # Add key and value to the dictionary.
        argv = argv[1:]  # Reduce the argument list by copying it starting from index 1.
    return opts


###########################################################
## Description: Checks the passed string for the quit phrase
###########################################################
def continueConvo(string):
    if quitPhrase in string:
        return False 
    else : 
        return True

###########################################################
## Description: The messenger class uses a port number and
## host name to set up a socket for sending and receiving
## message with a client
###########################################################
class Messenger:
    # stores the port number and host name upon init
    def __init__(self, pnum, hname):
        self.pnum = pnum
        self.hname = hname
    
    # set up the socket for listening
    def setUpSocket(self):
        self.serverSocket = socket(AF_INET, SOCK_STREAM)
        self.serverSocket.bind(("", self.pnum))
        self.serverSocket.listen(1)
        
    # wait for connection and incoming messages
    def waitAndReceive(self):
        self.connectionSocket, self.addr = self.serverSocket.accept()
        message = self.connectionSocket.recv(500)
        return message.decode()
    
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
        while len(fullMessage) < self.amnt :
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





if __name__ == '__main__':
    from sys import argv
    myargs = getopts(argv)
    if '-hostname' in myargs:
        hostname = myargs['-hostname']
        print("Using hostname:", myargs['-hostname'])
    else : 
        print("Using default hostname:", hostname)

    if '-port' in myargs:
        portnum = myargs['-port']
        print("Using port:", portnum)
    else : 
        print("Using default port:", portnum)

    # set up messenger device
    server = Messenger(int(portnum), hostname)

    # set up socket for TCP
    server.setUpSocket()

    print("The server is ready to receive")

    # connection stays up for receiving until server quits
    check1 = ""
    while continueConvo(check1):
        response = "Chatserver> Hi"
        # wait for connection and to get first message
        clientMessage = server.waitAndReceive()
        
        # print client message
        print(clientMessage)
        # print server response
        print(response)
        
        # conversation with client stays open
        # till client sends \\quit
        check2=""
        while continueConvo(check2):
            
            # send server response
            server.sendMessage(response)
            
            print("Waiting for client to respond back...")
            
            #server bailed
            if server.getIncomingAmount() == 0:
                check2 = "\quit"
            else :
                server.sendMessage("recv amnt")
                fullMessage = server.getMessage()
                print(fullMessage)
                serverOutput = input("Chatserver> ")
                response = "Chatserver> " + serverOutput
                
                #when the server has entered \quit
                if continueConvo(serverOutput) is False:
                    # send response
                    server.sendMessage(response)
                    # set both checks quit message
                    check2 = "\quit"
                    check1 = "\quit"
        server.close()
