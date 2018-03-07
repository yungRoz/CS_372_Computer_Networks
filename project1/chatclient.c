/*********************************************************************
** Author: Romano Garza
** Date: 02/11/18
** Description: Client program for chating with a server
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>

// input and message types
enum types {name=0, message, initialMessage, server, ignore, client};

struct Messenger
{
    int socketFD, portNumber, charsWritten, charsRead, amount, rightServer;
    int serverHasntLeft;
    int clientHasntLeft;
    char serverBuffer[501], clientBuffer[501], amountBuffer[10], clientName[501], spare[501], greeting[5];
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
};
struct Messenger m;

/*********************************************************************
** Description: error function used to display error messages
*********************************************************************/
void error(const char *msg) { perror(msg); exit(EXIT_FAILURE); } // Error function used



/*********************************************************************
 ** Description: sends the amount being sent from the client to the
 ** server
 *********************************************************************/
void sendAmount(int amnt)
{
    // clear out amount to send buffer
    memset(m.amountBuffer,'\0', 10);
    // store amount in amountBuffer
    sprintf(m.amountBuffer, "%d", amnt);
    // send amount buffer to the server
    m.charsWritten = (int)send(m.socketFD, m.amountBuffer, strlen(m.amountBuffer) + 1, 0);
    if (m.charsWritten < 0) error("CLIENT: ERROR sending amount");
}


/*********************************************************************
 ** Description: gets back messages from server, stores in global
 ** serverBuffer
 *********************************************************************/
void getResponse(int type)
{
    // clear out buffer
    memset(m.serverBuffer, '\0', sizeof(m.serverBuffer));
    m.charsRead = recv(m.socketFD, m.serverBuffer, sizeof(m.serverBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
    if (m.charsRead < 0) error("CLIENT: ERROR reading from socket");
    
    // print responses that shouldn't be ignored
    if(type!=ignore) printf("\n%s", m.serverBuffer);
    
    // look for quit
    if(strstr(m.serverBuffer, "\\quit")!=NULL)
    {
        m.serverHasntLeft = 0;
    }

}


/*********************************************************************
** Description: get input from the command line, either the users 
** name or a message to send to the server. If getting the message
** to send to the server, preprocess the string for sending.
*********************************************************************/
//void disconnect(){close(socketFD);}
void getInput(int type)
{
    if(type == name)
    {
        // clear out the clientName buffer where the clients name gets stored
        memset(m.clientName, '\0', sizeof(m.clientName));
        printf("Enter your name to start chat: ");
        // get client name
        fgets(m.clientName, 500, stdin);
        //remove new line
        //buffer[strcspn(m.clientName, "\n")] = 0;
        // append ">"
        int len = strlen(m.clientName);
        m.clientName[len-1] = '>';
        m.clientName[len] = ' ';
    }
    else if(type == message)
    {
        // clear out the clientBuffer where the full message is sent
        memset(m.clientBuffer, '\0', sizeof(m.clientBuffer));
        // print client name
        printf("\n%s", m.clientName);
        //get message into client buffer
        fgets(m.clientBuffer, 501, stdin);
        // look for quit
        if(strstr(m.clientBuffer, "\\quit")!=NULL)
        {
            // send server an amount flag
            sendAmount(1000);
            // wait for response
            getResponse(server);
            m.clientHasntLeft = 0;
        }
    }

}

/*********************************************************************
 ** Description: appends to spare buffer the appropriate type before
 ** it is sent
 *********************************************************************/
void appendToMessage(int type)
{
    int i = 0;
    int j = 0;
    
    char* toAppend;
    // get starting point for appending
    while(m.spare[i]!='\0')
    {
        i++;
    }
    
    // get type to append
    if(type == name)
    {
        toAppend = m.clientName;
    }
    else if(type == initialMessage)
    {
        toAppend = m.greeting;
    }
    else if(type == message)
    {
        toAppend = m.clientBuffer;
    }
    
    //append the message
    for(j; toAppend[j] != '\0'; j++)
    {
        m.spare[i] = toAppend[j];
        i++;
    }
    
}

/*********************************************************************
** Description: send first message to server with client name prepended
** depending on type: initialMessage or message
*********************************************************************/
void sendMessage(int type)
{
    int i, j;
    // First prepend client name
    memset(m.spare, '\0', sizeof(m.spare));
    for(i = 0; m.clientName[i]!='\0'; i++)
    {
        m.spare[i] = m.clientName[i];
    }

    // next append the message
    if(type == initialMessage)
    {
        sprintf(m.spare + strlen(m.spare), " Hey");
        printf("%s", m.spare);
    }
    else
    {
        sprintf(m.spare + strlen(m.spare), m.clientBuffer);
    }
    
    m.charsWritten = send(m.socketFD, m.spare, strlen(m.spare)+1, 0);
    
    // Check for errors
    if (m.charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (m.charsWritten < strlen(m.spare)+1)
        printf("CLIENT: WARNING: Not all data written to socket!\n");
    // Print message to screen
    //printf("%s> Hey\n", m.clientName);
}




/*********************************************************************
** Description: checks if message is from correct server via
** global serverBuffer
*********************************************************************/
int validateResponse()
{
    if(strcmp(m.serverBuffer, "Chatserver> Hi") == 0) return 1;
    else return 0;
}




/*********************************************************************
** Description: checks if message contains "\quit" and responds 
** appropriately based on who has sent it
*********************************************************************/
int quit(int type)
{
    // quit not found, keep going
    if(strstr(m.clientBuffer, "\\quit")==NULL) return 0;
    if(strstr(m.serverBuffer, "\\quit")== NULL) return 0;
    
    // quit found, stop
    if(type == client)
    {
        // send server an amount flag
        sendAmount(1000);
        // wait for response
        getResponse(server);
    }
    return 1;
}


/*********************************************************************
** Description: returns amount to send to server
*********************************************************************/
int getAmount()
{
    return (int)strlen(m.clientBuffer) + 1 + (int)strlen(m.clientName);
}



/*********************************************************************
** Description: set up the server connection that will begin
*********************************************************************/
void setUpS()
{
     // Clear out the address struct
    memset((char*)&m.serverAddress, '\0', sizeof(m.serverAddress));
    // Create a network-capable socket
    m.serverAddress.sin_family = AF_INET;
    // Store the port number
    m.serverAddress.sin_port = htons(m.portNumber);
    // Copy in the address
    memcpy((char*)&m.serverAddress.sin_addr.s_addr, (char*)m.serverHostInfo->h_addr, m.serverHostInfo->h_length);
    // Set up the socket
    m.socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (m.socketFD < 0) error("CLIENT: ERROR opening socket");

    // Connect to server address
    if (connect(m.socketFD, (struct sockaddr*)&m.serverAddress, sizeof(m.serverAddress)) < 0)
        error("CLIENT: ERROR connecting");
}


int main(int argc, char *argv[])
{
    /*int socketFD, portNumber, charsWritten, charsRead, amount;
    char serverBuffer[514], clientBuffer[514], amountBuffer[10], clientName[514];
    
    char *quitphrase = "\\quit";
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    int cont = 1;*/
    
    
    int amnt;
    // check argument count
    if (argc < 2) {
        fprintf(stderr,"USAGE: %s hostname port\n", argv[0]);
        exit(0);
    }
    else { // set up portNumber and hostName
        m.portNumber = atoi(argv[2]);
        //conver machine name into a special form address
        m.serverHostInfo = gethostbyname(argv[1]);
    }
    
    // get user name
    getInput( name );
    
    // set up server
    setUpS();
    
    // send first message
    sendMessage(initialMessage);
    // get first server message
    getResponse(server);
    
    // check if it's the right server
    if(validateResponse())
    {
        m.clientHasntLeft = 1;
        m.serverHasntLeft = 1;
        while(m.clientHasntLeft && m.serverHasntLeft)
        {
            
            getInput(message);
            if(m.clientHasntLeft)
            {
                // get amount of client message
                amnt = getAmount();
                // send amount message to server
                sendAmount(amnt);
                // get amount response from server
                getResponse(ignore);
                
                // send message to server
                sendMessage(message);
                // get message from server
                getResponse(server);
            }
        }
    }
    else{
        error("ERROR CONNECTING TO SERVER");
    }
    close(m.socketFD);

}
