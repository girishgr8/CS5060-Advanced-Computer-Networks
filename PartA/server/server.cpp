#include <iostream>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#define PORT 9001
#define ONE 1
#define ZERO 0
#define SUCCESS 1
#define FAILURE -1
#define BACKLOG 5
#define TIMEVALSECS 10
#define TIMEVALUSECS 0
#define MAXCLIENTS 5
#define FLAGS 0
#define MAXBUFFERSIZE 2048
#define CHARSIZE sizeof(char)
using namespace std;

struct sockaddr_in srv;
FD_SET fr, fw, fe;
int maxFd;
int clientsInQueue = 0;

int clientSockFd[MAXCLIENTS];

// sendFileToClient() function reads the requested file from the local directory
// It iteratively reads no. of bytes = buffersize at one go and sends those many bytes to client
// If the file ptr has reached "End-Of-File(EOF)" then indicate the same to client by send "FEND" flag.
int sendFileToClient(FILE *fp, char buffer[], int clnSockFd)
{
    memset(buffer, 0, MAXBUFFERSIZE);
    // send FEBGIN" to mark the beginning of file transfer
    buffer[0] = 'F';
    buffer[1] = 'B';
    buffer[2] = 'E';
    buffer[3] = 'G';
    buffer[4] = 'I';
    buffer[5] = 'N';
    buffer[6] = '\0';

    int retCode = send(clnSockFd, buffer, strlen(buffer), FLAGS);

    // Read the size of file (in terms of number of bytes)
    fseek(fp, 0, SEEK_END);
    const long fileSize = ftell(fp);
    cout << "Filesize (in Bytes) = " << fileSize << endl;
    rewind(fp);

    int totalBytesRead = 0;
    char writeBuffer[MAXBUFFERSIZE] = {0};

    // read until file ptr reached EOF
    while (!feof(fp))
    {
        int bytesRead = fread(writeBuffer, ONE, MAXBUFFERSIZE, fp);
        totalBytesRead += bytesRead;
        int retCode = send(clnSockFd, writeBuffer, bytesRead, FLAGS);
        memset(writeBuffer, 0, bytesRead);
        if (retCode < ZERO)
        {
            cout << "Error in sending file to client" << clnSockFd << endl;
            break;
        }
        Sleep(50);
    }
    memset(writeBuffer, 0, MAXBUFFERSIZE);
    // send "FEND" to mark the ending of file transfer
    writeBuffer[0] = 'F';
    writeBuffer[1] = 'E';
    writeBuffer[2] = 'N';
    writeBuffer[3] = 'D';
    writeBuffer[4] = '\0';
    retCode = send(clnSockFd, writeBuffer, strlen(writeBuffer), FLAGS);

    cout << "Requested file sent to client: " << clnSockFd << endl;
    fclose(fp);
    return SUCCESS;
}

void processNewMsgFromClient(int clnSockFd)
{
    char buffer[MAXBUFFERSIZE];
    FILE *fp;
    memset(buffer, 0, MAXBUFFERSIZE);
    cout << "Processing new message from the existing client: " << clnSockFd << endl;
    int retCode = recv(clnSockFd, buffer, MAXBUFFERSIZE, FLAGS);
    if (retCode < ZERO)
    {
        cout << "Connection aborted. Closing the connection for client: " << clnSockFd << endl;
        // If connection was aborted, remove the client sockFd from the queue of clients
        if (WSAGetLastError() == WSAECONNABORTED)
        {
            closesocket(clnSockFd);
            for (int i = 0; i < MAXCLIENTS; i++)
            {
                if (clientSockFd[i] == clnSockFd)
                {
                    clientSockFd[i] = 0;
                    clientsInQueue--;
                    break;
                }
            }
            Sleep(50);
        }
    }
    else
    {
        cout << "New Message received from client is: " << buffer << endl;
        fp = fopen(buffer, "rb");
        memset(buffer, 0, MAXBUFFERSIZE);
        if (fp == NULL)
        {
            cout << "Cannot send the requested object from client " << clnSockFd << ". File does not exist !" << endl;
            const char *errMsg = "File not exist";
            int errLen = strlen(errMsg);
            retCode = send(clnSockFd, errMsg, errLen, FLAGS);
            return;
        }
        retCode = sendFileToClient(fp, buffer, clnSockFd);
        if (retCode == SUCCESS)
        {
            // Sending response back to the client
            const char *msg = "File successfully sent to client : ";
            int msgLen = strlen(msg);
            send(clnSockFd, msg, msgLen, FLAGS);
            cout << "*****************************************************" << endl;
        }
    }
}

void processClientRequest(int sockFd)
{
    if (FD_ISSET(sockFd, &fr))
    {
        int clnAddrLen = sizeof(struct sockaddr);
        if (clientsInQueue == MAXCLIENTS)
        {
            cout << "Server is busy with existing clients, new clients cannot be handled !" << endl;
        }
        fflush(stdout);
        int clnSockFd = accept(sockFd, NULL, &clnAddrLen);
        // if some client is accepted successfully, then put that client's socket descriptor in the array
        if (clnSockFd > ZERO)
        {
            for (int i = 0; i < MAXCLIENTS; i++)
            {
                if (clientSockFd[i] == 0)
                    clientSockFd[i] = clnSockFd;
            }
            const char *buffer = "Connection setup successful";
            int len = strlen(buffer);
            send(clnSockFd, buffer, len, FLAGS);
        }
    }
    else
    {
        // Got a new message from the existing connected client
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (FD_ISSET(clientSockFd[i], &fr))
            {
                processNewMsgFromClient(clientSockFd[i]);
            }
        }
    }
}

int main()
{
    int retCode;

    // Step 1: Initialise WSA variable
    // The WSACleanup() function terminates use of the Winsock DLL library
    // The WSAStartup()function initiates use of the Winsock DLL by a process
    WSADATA ws;

    if (WSAStartup(MAKEWORD(2, 2), &ws) < ZERO)
    {
        cout << "WSA Startup Failed" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "WSA Startup Initialised" << endl;
    }

    // Step 2: Initialising the socket
    // The socket() call by default creates a "blocking" socket.
    int sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // socket() system call returns -1 if there was a problem in opening a socket.
    if (sockFd < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket opened successfully with socket descriptor id: " << sockFd << endl;
    }

    // Initialising the environment variable for sockaddr structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT); // i.e. specify the network byte order when specifying the port number

    // Method 1: The below command will set the server's IP address same as system's current IP address
    srv.sin_addr.s_addr = INADDR_ANY;

    // Method 2: The below command will set the "Loopback IP address" of system in the long format in network byte order
    // srv.sin_addr.s_addr = inet_addr("127.0.0.1");

    // sin_zero is a char[8] array where we will set all the values to zero
    memset(&(srv.sin_zero), 0, 8);

    // Step 3: Binding the socket to the local port
    retCode = bind(sockFd, (sockaddr *)&srv, sizeof(sockaddr));

    if (retCode < ZERO)
    {
        cout << "Error: Failed in binding socket to local port" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket binded to local port successfully" << endl;
    }

    // Step 4: Listening to the new requests from client (queueing the client's requests)

    // The 'backlog' parameter defines the maximum length to which the queue of pending connections for sockFd may grow.
    // If a connection request arrives when the queue is full,
    // the client may receive an error with an indication of ECONNREFUSED
    retCode = listen(sockFd, BACKLOG);

    if (retCode < ZERO)
    {
        cout << "Error: Failed in listening to local port !" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Started listening to local port successfully !" << endl;
    }

    maxFd = sockFd;

    // Step 5: Keep listening to new incoming request and start serving the existing requests in queue
    while (TRUE)
    {
        // clear the FD sets for read, write and exception socket descriptors
        FD_ZERO(&fr);
        FD_ZERO(&fw);
        FD_ZERO(&fe);

        FD_SET(sockFd, &fr);
        FD_SET(sockFd, &fe);

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (clientSockFd[i] != 0)
            {
                FD_SET(clientSockFd[i], &fr);
                FD_SET(clientSockFd[i], &fe);
            }
        }

        struct timeval tval;
        tval.tv_sec = TIMEVALSECS;
        tval.tv_usec = TIMEVALUSECS;

        // select() call unsets the FD_SETs
        int readySocketFd = select(maxFd + 1, &fr, &fw, &fe, &tval);

        // retCode contains the total number of socket handles that are ready and contained in the FD_SET structures
        if (readySocketFd > ZERO)
        {
            cout << "New client request arrived !" << endl;
            // New client request arrived
            processClientRequest(sockFd);
        }
        else if (readySocketFd == ZERO)
        {
            cout << "No client's request arrived on port: " << PORT << endl;
        }
        readySocketFd = 0;
    }
}
