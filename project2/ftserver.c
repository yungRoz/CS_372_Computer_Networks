/*********************************************************************
** Author: Romano Garza
** Date: 02/11/18
** Description: Server program for chating with a server
*********************************************************************/
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>

//#include "socketHelpers.h"
// input and message types
enum types {name=0, port, message, initialMessage, server,
            ignore, client, command, list, get, sendError, confirmation,
            data, amount, directory, file, hostName, filename};

struct  Ftserver
{
        int listenSocketFD, establishedConnectionFD, dataSocketFD, portNumber, cmnd;
        int charsRead, charsReadKey, charsReadTxt, dirCount, charsWritten;
        int pid;
        int i;
        int option;
        socklen_t sizeOfClientInfo;
        struct hostent* serverHostInfo;
        struct sockaddr_in serverAddress, clientAddress;
        char buffer[1500], portBuffer[20], commandBuffer[5], amountBuffer[10];
        char fileNameBuffer[20], dirBuffer[20000], fileBuffer[100000];
        char hostNameBuffer[10], originPortBuffer[10], ipBuffer[100];

};


struct Ftserver s;

/*********************************************************************
** Description: error function used to display error messages
*********************************************************************/
void error(const char *msg) {
        perror(msg); exit(EXIT_FAILURE);
}


/*********************************************************************
** Description: convert the hostname to ip address
** source: https://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
*********************************************************************/
int hostname_to_ip()
{
        struct hostent *he;
        struct in_addr **addr_list;
        int i;

        if ( (he = gethostbyname( s.hostNameBuffer ) ) == NULL)
        {
                // get the host info
                herror("gethostbyname");
                return 1;
        }

        addr_list = (struct in_addr **) he->h_addr_list;
        memset(s.ipBuffer, '\0', sizeof(s.ipBuffer));
        for(i = 0; addr_list[i] != NULL; i++)
        {
                //Return the first one;
                strcpy(s.ipBuffer, inet_ntoa(*addr_list[i]) );
                printf("IP: %s\n", s.ipBuffer);
                return 0;
        }

        return 1;
}

/*********************************************************************
** Description: gets back messages from server, stores in global
** serverBuffer
*********************************************************************/
void getResponse(int type)
{

        if(type == port) {
                memset(s.portBuffer, '\0', sizeof(s.portBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.portBuffer,
                                   sizeof(s.portBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                printf("Requested port %s\n", s.portBuffer);
                // change portNumber for upcoming dataSocket
                s.portNumber = atoi(s.portBuffer);
        }
        else if(type == command) {
                memset(s.commandBuffer, '\0', sizeof(s.commandBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.commandBuffer,
                                   sizeof(s.commandBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                printf("Command %s\n");
                if( strcmp(s.commandBuffer, "-l") == 0) {
                        printf("List directory requested on port %s. \n", s.portBuffer);
                }
        }
        else if(type == hostName) {

                memset(s.hostNameBuffer, '\0', sizeof(s.hostNameBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.hostNameBuffer,
                                   sizeof(s.hostNameBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                printf("Connection from %s\n", s.hostNameBuffer);
                strcat(s.hostNameBuffer, ".engr.oregonstate.edu");
                if(hostname_to_ip()) error("SERVER: ERROR transforming hostname to ip\n");
                //s.serverHostInfo = gethostbyname(s.hostNameBuffer);
        }
        else if(type == filename) {
                memset(s.fileNameBuffer, '\0', sizeof(s.fileNameBuffer));
                strncpy(s.fileNameBuffer, s.buffer, sizeof(s.buffer));
                printf("File \"%s\" requested on port %s.\n", s.fileNameBuffer, s.portBuffer);
        }
        else if(type == ignore){
                memset(s.buffer, '\0', sizeof(s.buffer));
                s.charsRead = recv(s.establishedConnectionFD, s.buffer, sizeof(s.buffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
        }

}

/*********************************************************************
** Description: send first message to server with client name prepended
** depending on type: initialMessage or message
*********************************************************************/
void sendMessage(int type){
        // Check for errors
        if(type == directory) {
                // get buffer amount
                int amount = (int)strlen(s.dirBuffer);
                // clear out amount to send buffer
                memset(s.amountBuffer,'\0', 10);
                // store amount in amountBuffer
                sprintf(s.amountBuffer, "%d", amount);
                // send amount buffer to the server
                s.charsWritten = (int)send(s.dataSocketFD, s.amountBuffer,
                                           strlen(s.amountBuffer) + 1, 0);

                if (s.charsWritten < 0) error("CLIENT: ERROR sending amount");
                s.charsWritten = send(s.dataSocketFD, s.dirBuffer, strlen(s.dirBuffer), 0);
                int checkSend = -5;
                do {
                        // check send buffer for the dataSocket
                        // loops until send buffer is empty
                        ioctl(s.dataSocketFD, TIOCOUTQ, &checkSend);
                } while(checkSend > 0);

        }
        else if( type == file) {
                s.charsWritten = send(s.dataSocketFD, s.fileBuffer, strlen(s.fileBuffer), 0);
                int checkSend = -5;
                do {
                        // check send buffer for the dataSocket
                        // loops until send buffer is empty
                        ioctl(s.dataSocketFD, TIOCOUTQ, &checkSend);
                } while(checkSend > 0);
        }
        else if( type == confirmation) {
                if(s.cmnd == sendError) {
                        s.charsWritten = send(s.establishedConnectionFD, "22", 3, 0);
                        getResponse(ignore);
                        s.charsWritten = send(s.establishedConnectionFD,
                                              "ERROR: invalid command", 23, 0);
                }
                else {
                        s.charsWritten = send(s.establishedConnectionFD, "3", 2, 0);
                        getResponse(ignore);
                        s.charsWritten = send(s.establishedConnectionFD, "Got", 4, 0);
                }
        }

        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");
}


/*********************************************************************
** Description: set up the server socket for listening
*********************************************************************/
void setUpSListen()
{
        // Clear out the address struct
        memset((char*)&s.serverAddress, '\0', sizeof(s.serverAddress));
        // Create a network-capable socket
        s.serverAddress.sin_family = AF_INET;
        // Store the port number
        s.serverAddress.sin_port = htons(s.portNumber);
        s.serverAddress.sin_addr.s_addr = INADDR_ANY;
        //setUpSListen();
        s.option=1;
        //set up the socekt depending on type :
        // create the socket, check for muteAllErrors

        s.listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
        if(s.listenSocketFD < 0) error("Server: ERROR opening socket");
        // set socket level option to allow reuse of local addresses
        setsockopt(s.listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &s.option, sizeof(s.option));
        //enable socket to begin listening
        if(bind(s.listenSocketFD, (struct sockaddr *)&s.serverAddress,
                sizeof(s.serverAddress)) < 0) {
                error("Server: ERROR on binding");
        }
        // flip socket on and all to receive up to 5 connects
        if(listen(s.listenSocketFD, 5) == -1) error("Server: ERROR listening.");

        //s.sizeOfClientInfo = sizeof(s.clientAddress);
}
/*********************************************************************
** Description: set up the server to connect to the client for data
** transmission
*********************************************************************/
void setUpSConnect()
{
        // Clear out the address struct
        memset((char*)&s.serverAddress, '\0', sizeof(s.serverAddress));
        // Create a network-capable socket
        s.serverAddress.sin_family = AF_INET;
        // Store the port number
        s.serverAddress.sin_port = htons(s.portNumber);
        s.serverAddress.sin_addr.s_addr = inet_addr(s.ipBuffer);

        // Set up the socket
        s.dataSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
        if (s.dataSocketFD < 0) error("CLIENT: ERROR opening socket");

        // Connect to server address
        if (connect(s.dataSocketFD, (struct sockaddr*)&s.serverAddress, sizeof(s.serverAddress)) < 0)
                error("CLIENT: ERROR connecting");
}

void validate(int type){
        if(type == command) {
                if(strcmp(s.commandBuffer, "-l") == 0) s.cmnd = list;
                else if(strcmp(s.commandBuffer, "-g") == 0) s.cmnd = get;
                else s.cmnd = sendError;
        }
}

/*********************************************************************
** Description: opens directory and appends all fileNames
**
*********************************************************************/
/* Source: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program*/
void getDirList(){
        DIR *d;
        struct dirent *dir;
        d = opendir(".");

        if (d) {
                memset(s.dirBuffer, '\0', sizeof(s.dirBuffer));
                while((dir = readdir(d)) !=NULL) {
                        /*printf("%s\n", dir->d_name);
                           strcpy(s.dirBuffer[strlen(s.dirBuffer)-1], dir->d_name);
                           strcpy(s.dirBuffer[strlen(s.dirBuffer)-1], " ");*/
                        strcat(s.dirBuffer, dir->d_name);
                        strcat(s.dirBuffer, " ");
                }
                closedir(d);
        }
}

void getText(){
        long fsize;
        FILE *fp = fopen(s.fileNameBuffer, "rb");
        printf("filename: %s", filename);
        if(fp != NULL) {
                fseek(fp, 0, SEEK_END);
                fsize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                //allocate space for txt string
                memset(s.fileBuffer, '\0', sizeof(s.fileBuffer));
                fread(s.fileBuffer, fsize, 1, fp);
                if( ferror(fp) !=0) {
                        error("SERVER: ERROR reading plain text file\n");
                }
                //strip new line, replace with end of line characters
                s.fileBuffer[fsize-1] = '\0';
                s.fileBuffer[fsize] = '\0';
                fclose(fp);
        }
        else{
                printf("File not found sending error message to %s:%s",
                       s.hostNameBuffer, s.originPortBuffer);
                memset(s.fileBuffer, '\0', sizeof(s.fileBuffer));
                strcat(s.fileBuffer, "FILE NOT FOUND" );
        }
}

/*********************************************************************
** Description: waits for server to accept a connection, then calls
** function for subsequent processing
*********************************************************************/
void acceptConnections(){
        while(1) {
                printf("Waiting for connection...\n");
                s.establishedConnectionFD = accept(s.listenSocketFD, NULL, NULL);
                if(s.establishedConnectionFD < 0) error("Server: ERROR on accept");
                printf("Received connection!!\n");
                // fork process for multi-threading support
                //s.pid = fork();
                // if no errors in forking
                if(1) { //s.pid==0) {
                       //close the socket we waited on
                        close(s.listenSocketFD);
                        // get the client host name
                        getResponse(hostName);
                        sendMessage(confirmation);
                        // display connection from message
                        //printf("Connection from %s\n", s.hostNameBuffer);
                        // get the request port number
                        getResponse(port);
                        sendMessage(confirmation);
                        // get the command arguments
                        getResponse(command);
                        // validate command
                        validate(command);
                        // send confirmation of receipt
                        sendMessage(confirmation);

                        if(s.cmnd == list) {
                                setUpSConnect();
                                //get list of directory contents
                                getDirList();
                                //send initial message connecting to client
                                sendMessage(confirmation);
                                // client sends back request for
                                // directories, just ignore because they will be
                                // sent
                                getResponse(ignore);
                                sendMessage(directory);
                        }
                        else if(s.cmnd == get) {
                                setUpSConnect();
                                getText();
                                sendMessage(confirmation);
                                getResponse(filename);
                                sendMessage(confirmation);
                                getResponse(ignore);
                                sendMessage(file);
                        }
                        else{
                                error("ERROR: validating command.");
                        }
                        close(s.dataSocketFD);
                        close(s.establishedConnectionFD);
                }
        }
}




int main(int argc, const char* argv[]){
        //test getDirList();
        //int i = 0;
        //getDirList();
        //printf("%s", s.dirBuffer);
        /*/ /core dumps in terminal   // can be fixed by initializing
        //array of strings with null values
        while(s.dirBuffer[i]!= NULL) {
                printf("string: %s  string_size:%d \n", s.dirBuffer[i], (int)sizeof(s.dirBuffer[i]));
                i++;
        } */

        /* test getText(); */
        //getText("textfile.txt");
        //printf("\n\nHere's what was read: %s\n\n", s.fileBuffer);
        // check argument count
        if (argc < 2) {
                fprintf(stderr,"USAGE: %s portNumber\n", argv[0]);
                exit(0);
        }
        else {    // set up portNumber and hostName
                s.portNumber = atoi(argv[1]);
                memset(s.originPortBuffer, '\0', sizeof(s.originPortBuffer));
                strcat(s.originPortBuffer, argv[1]);
                printf("Server open on %s\n", argv[1]);
        }
        //set up server for listening
        //setUpS();
        setUpSListen();
        //wait for and process connections
        acceptConnections();





}
