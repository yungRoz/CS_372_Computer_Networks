/*********************************************************************
** Author: Romano Garza
** Date: 02/11/18
** Description: Server program for file transfer with a client
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

// types for message sending and receiving
enum types {name=0, port, message, initialMessage, server,
            ignore, client, command, list, get, sendError, confirmation,
            data, amount, directory, file, hostName, filename};

struct  Ftserver
{
        int listenSocketFD, establishedConnectionFD, dataSocketFD, portNumber, cmnd;
        int charsRead, charsReadKey, charsReadTxt, dirCount, charsWritten, noError;
        int pid;
        int i;
        int option;
        socklen_t sizeOfClientInfo;
        struct hostent* serverHostInfo;
        struct sockaddr_in serverAddress, clientAddress;
        char iPortBuffer[10];
        char buffer[1500], fileNameErrScrnMessage[100], successFileMessage[100], successDirMessage[100], portBuffer[20], commandBuffer[5], amountBuffer[10];
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
                //printf("IP: %s\n", s.ipBuffer);
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
        // response types are split up by
        // the buffer they use along with the
        // socket they use to receive messages from
        // but all follow a general order: buffer is cleared out,
        // message from client is received into the buffers
        // appropriate messenger variables are set
        if(type == port) {
                memset(s.portBuffer, '\0', sizeof(s.portBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.portBuffer,
                                   sizeof(s.portBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                // change portNumber for upcoming dataSocket
                s.portNumber = atoi(s.portBuffer);
                // cat the portnumber to screen messages
                strcat(s.successDirMessage, s.portBuffer);
                strcat(s.successFileMessage, s.portBuffer);
        }
        else if(type == command) {
                memset(s.commandBuffer, '\0', sizeof(s.commandBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.commandBuffer,
                                   sizeof(s.commandBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                //printf("Command %s\n");
                if( strcmp(s.commandBuffer, "-l") == 0) {
                        printf("List directory requested on port %s. \n", s.portBuffer);
                }
        }
        else if(type == hostName) {

                memset(s.hostNameBuffer, '\0', sizeof(s.hostNameBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.hostNameBuffer,
                                   sizeof(s.hostNameBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                printf("\n\nConnection from %s\n",s.hostNameBuffer);
                // store hostname in all display message buffers
                // this is to avoid a bug in hostname_to_ip
                memset(s.fileNameErrScrnMessage,'\0', sizeof(s.fileNameErrScrnMessage));
                memset(s.successFileMessage, '\0', sizeof(s.successFileMessage));
                memset(s.successDirMessage, '\0', sizeof(s.successDirMessage));
                sprintf(s.successFileMessage,"Sending file contents to %s:", s.hostNameBuffer);
                sprintf(s.successDirMessage, "Sending directory contents to %s:", s.hostNameBuffer);
                sprintf(s.fileNameErrScrnMessage, "File not found sending error message to %s:", s.hostNameBuffer);
                strcat(s.fileNameErrScrnMessage, s.iPortBuffer);
                //printf("Connection from %s\n", s.hostNameBuffer);
                strcat(s.hostNameBuffer, ".engr.oregonstate.edu");



                if(hostname_to_ip()) error("SERVER: ERROR transforming hostname to ip\n");
                //s.serverHostInfo = gethostbyname(s.hostNameBuffer);
        }
        else if(type == filename) {
                memset(s.fileNameBuffer, '\0', sizeof(s.fileNameBuffer));
                s.charsRead = recv(s.establishedConnectionFD, s.fileNameBuffer, sizeof(s.fileNameBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
                printf("File \"%s\" requested on port %s.\n", s.fileNameBuffer, s.portBuffer);
        }
        else if(type == ignore) {
                memset(s.buffer, '\0', sizeof(s.buffer));
                s.charsRead = recv(s.establishedConnectionFD, s.buffer, sizeof(s.buffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
        }
        else if(type == data) {
                memset(s.buffer, '\0', sizeof(s.buffer));
                s.charsRead = recv(s.dataSocketFD, s.buffer, sizeof(s.buffer) - 1, 0); // Read data from the socket, leaving \0 at end
                if (s.charsRead < 0) error("CLIENT: ERROR reading from socket");
        }

}

/*********************************************************************
** Description: send messages to client, type denotes the socket
** and buffer to send
*********************************************************************/
void sendMessage(int type){
        // Each
        if(type == directory) {
                getDirList();
                printf("%s\n",s.successDirMessage);
                // get buffer amount
                int amount = (int)strlen(s.dirBuffer);
                //printf("amount %d\n", amount);
                // clear out amount to send buffer
                memset(s.amountBuffer,'\0', sizeof(s.amountBuffer));
                // store amount in amountBuffer
                sprintf(s.amountBuffer, "%d", amount);
                //send amount
                s.charsWritten = (int)send(s.dataSocketFD, s.amountBuffer,
                                           strlen(s.amountBuffer)+1, 0);
                if (s.charsWritten < 0) error("CLIENT: ERROR sending amount");
                // get response
                getResponse(data);
                // send directory contents
                s.charsWritten = send(s.dataSocketFD, s.dirBuffer, strlen(s.dirBuffer)+1, 0);
                if (s.charsWritten < 0) error("CLIENT: ERROR sending amount");

        }
        else if( type == file) {
                int amount = (int)strlen(s.fileBuffer);
                // clear out amount to send buffer
                memset(s.amountBuffer,'\0', sizeof(s.amountBuffer));
                // store amount in amountBuffer
                sprintf(s.amountBuffer, "%d", amount);
                // send amount
                s.charsWritten = (int)send(s.dataSocketFD, s.amountBuffer,
                                           strlen(s.amountBuffer)+1, 0);
                if (s.charsWritten < 0) error("CLIENT: ERROR sending amount");
                // get response
                getResponse(data);
                // send file contents
                s.charsWritten = send(s.dataSocketFD, s.fileBuffer, strlen(s.fileBuffer)+1, 0);
                if (s.charsWritten < 0) error("CLIENT: ERROR sending amount");
        }
        else if( type == filename) {
                int amount = (int)strlen(s.fileBuffer);
                memset(s.amountBuffer, '\0', sizeof(s.amountBuffer));
                sprintf(s.amountBuffer, "%d", amount);
                // send amount
                s.charsWritten = (int)send(s.establishedConnectionFD, s.amountBuffer,
                                           strlen(s.amountBuffer)+1, 0);
                // get response
                getResponse(ignore);
                // send error message
                s.charsWritten = send(s.establishedConnectionFD,
                                      s.fileBuffer, strlen(s.fileBuffer)+1, 0);
        }
        else if( type == confirmation) {
                // respond with error message if invalid command is passed
                if(s.cmnd == sendError) {
                        s.charsWritten = send(s.establishedConnectionFD, "22", 3, 0);
                        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");
                        getResponse(ignore);
                        s.charsWritten = send(s.establishedConnectionFD,
                                              "ERROR: invalid command", 23, 0);
                        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");

                }
                else { // confirm receipt of message
                        s.charsWritten = send(s.establishedConnectionFD, "3", 2, 0);
                        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");
                        getResponse(ignore);
                        s.charsWritten = send(s.establishedConnectionFD, "Got", 4, 0);
                        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");
                }
        }

        if (s.charsWritten < 0) error("CLIENT: ERROR writing to socket");
}

/*********************************************************************
** Description: getText opens the file given by the fileNameBuffer
** if file cannot be opened it, sends the appropriate message to the
** client
*********************************************************************/
void getText(){
        long fsize;
        FILE *fp = fopen(s.fileNameBuffer, "rb+");
        // printf("filename: %s", filename);
        if(fp != NULL) {
                fseek(fp, 0, SEEK_END);
                fsize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                // allocate space for file contents
                memset(s.fileBuffer, '\0', sizeof(s.fileBuffer));
                // read in file contents
                fread(s.fileBuffer, fsize, 1, fp);
                if( ferror(fp) !=0) {
                        error("SERVER: ERROR reading plain text file\n");
                }
                // strip new line, replace with end of line characters
                s.fileBuffer[fsize-1] = '\0';
                s.fileBuffer[fsize] = '\0';
                fclose(fp);
                // display success message on server screen
                printf("%s\n", s.successFileMessage);
                s.noError = 1;
                sendMessage(confirmation);
        }
        else{
                // display error message on server screen
                printf("%s\n", s.fileNameErrScrnMessage);
                // file file buffer with error message for sending
                memset(s.fileBuffer, '\0', sizeof(s.fileBuffer));
                strcat(s.fileBuffer, "FILE NOT FOUND" );
                s.noError = 0;
                // send file buffer filename error message to client
                sendMessage(filename);
        }
}

/*********************************************************************
** Description: set up the server socket for listening, used for initial
** socket set up
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
        s.option=1;

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

}

/*********************************************************************
** Description: evaluates passed command, sets command with sendError
** flag of incorrect command was passed
*********************************************************************/
void validate(int type){
        if(type == command) {
                if(strcmp(s.commandBuffer, "-l") == 0) s.cmnd = list;
                else if(strcmp(s.commandBuffer, "-g") == 0) s.cmnd = get;
                else s.cmnd = sendError;
        }
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
        //printf("Connecting to client\n");
        // Connect to server address
        if (connect(s.dataSocketFD, (struct sockaddr*)&s.serverAddress, sizeof(s.serverAddress)) < 0)
                error("CLIENT: ERROR connecting");


}

/*********************************************************************
** Description: waits for server to accept a connection, then calls
** function for subsequent processing
*********************************************************************/
void acceptConnections(){
        while(1) {
                //printf("Waiting for connection...\n");
                //accept connections from client
                s.establishedConnectionFD = accept(s.listenSocketFD, NULL, NULL);
                if(s.establishedConnectionFD < 0) error("Server: ERROR on accept");


                // get the client host name
                getResponse(hostName);
                // send confrimation message
                sendMessage(confirmation);
                // get the request port number
                getResponse(port);
                // send confirmation message
                sendMessage(confirmation);
                // get the command arguments
                getResponse(command);
                // validate command
                validate(command);
                // send confirmation of receipt
                sendMessage(confirmation);

                // if command is for file
                if(s.cmnd == get) {
                        // get the file name
                        getResponse(filename);
                        // get the file contents/check if it exists
                        getText();
                }

                // if comand is for directory
                if(s.cmnd == list) {
                        // this was for a bug where the c program was attempting
                        // to connect before the python program could set up a connection
                        sleep(1);
                        // set up a socket to connect to the client
                        setUpSConnect();
                        // send the directory
                        sendMessage(directory);
                        // close data socket
                        close(s.dataSocketFD);
                }
                else if(s.cmnd == get && s.noError) {
                        // same but for file contents
                        sleep(1);
                        setUpSConnect();
                        sendMessage(file);
                        close(s.dataSocketFD);

                }
                else if (s.cmnd == sendError) {
                        // client command error
                        printf("ERROR: validating command.\n");
                }
                // close first socket connection
                close(s.establishedConnectionFD);

        }
}




int main(int argc, const char* argv[]){

        if (argc < 2) {
                fprintf(stderr,"USAGE: %s portNumber\n", argv[0]);
                exit(0);
        }
        else {    // set up portNumber and hostName
                s.portNumber = atoi(argv[1]);
                memset(s.originPortBuffer, '\0', sizeof(s.originPortBuffer));
                strcat(s.originPortBuffer, argv[1]);
                memset(s.iPortBuffer, '\0', sizeof(s.originPortBuffer));
                strcat(s.iPortBuffer, argv[1]);
                printf("Server open on %s\n", argv[1]);
        }
        //set up server for listening
        setUpSListen();
        //wait for and process connections
        acceptConnections();

}
