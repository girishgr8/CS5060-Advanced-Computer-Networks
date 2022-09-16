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
#define MAXCLIENTS 2
#define MAXBUFFERSIZE 200000
#define CHARSIZE sizeof(char)
using namespace std;

struct sockaddr_in srv;
FD_SET fr, fw, fe;
int maxFd;
int clientsInQueue = 0;

int clientSockFd[MAXCLIENTS];

int sendFile(FILE *fp, int clnSockFd)
{
    char buffer[MAXBUFFERSIZE];
    memset(&buffer, 0, MAXBUFFERSIZE);

    // long fileSIZE;
    // fseek(fp, 0, SEEK_END);
    // fileSIZE = ftell(fp);
    // cout << "Filesize = " << fileSIZE << endl;
    // rewind(fp);
    int i = 1;
    while (fread(buffer, CHARSIZE, sizeof(buffer), fp) > ZERO)
    {
        int retVal = send(clnSockFd, buffer, strlen(buffer), 0);
        if (retVal < ZERO)
        {
            cout << "Error in sending file" << endl;
        }
        memset(&buffer, 0, MAXBUFFERSIZE);
        retVal = recv(clnSockFd, buffer, MAXBUFFERSIZE, 0);
        if (strcmp(buffer, "ACK"))
        {
            cout << "Packet " << (i++) << " received by client !" << endl;
            memset(&buffer, 0, MAXBUFFERSIZE);
        }
    }

    cout << "Upar dekho upar" << endl;
    buffer[0] = 'F';
    buffer[1] = 'E';
    buffer[2] = 'N';
    buffer[3] = 'D';
    int retVal = send(clnSockFd, buffer, strlen(buffer), 0);
    cout << "retVal = " << retVal << endl;
    cout << "Niche dekho niche" << endl;
    fclose(fp);
    return SUCCESS;
}

void processNewMsgFromClient(int clnSockFd)
{
    char buffer[MAXBUFFERSIZE];
    FILE *fp;
    memset(&buffer, 0, MAXBUFFERSIZE);
    cout << "Processing new message from the existing client: " << clnSockFd << endl;
    int retVal = recv(clnSockFd, buffer, MAXBUFFERSIZE, 0);
    if (retVal < ZERO)
    {
        cout << "Some error occurred. Closing the connection for client: " << clnSockFd << endl;
        closesocket(clnSockFd);
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (clientSockFd[i] == clnSockFd)
            {
                clientSockFd[i] = 0;
                break;
            }
        }
    }
    else
    {
        cout << "New Message received from client is: " << buffer;
        fp = fopen("./data.txt", "rb");
        if (fp == NULL)
        {
            cout << "Cannot send the requested object. File does not exist !" << endl;
        }
        memset(&buffer, 0, MAXBUFFERSIZE);
        buffer[0] = 'F';
        buffer[1] = 'B';
        buffer[2] = 'E';
        buffer[3] = 'G';
        buffer[4] = 'I';
        buffer[5] = 'N';
        retVal = send(clnSockFd, buffer, strlen(buffer), 0);
        // Sleep(3000);
        retVal = sendFile(fp, clnSockFd);
        if (retVal == SUCCESS)
        {
            // Sending response back to the client
            // const char *msg = "Processed your request";
            // int msgLen = strlen(msg);
            // send(clnSockFd, msg, msgLen, 0);
            cout << "*****************************************************" << endl;
        }
        else
        {
        }
    }
}

void processRequest(int sockFd)
{
    cout << "New client request arrived !" << endl;
    // New client request arrived
    if (FD_ISSET(sockFd, &fr))
    {
        int clnAddrLen = sizeof(struct sockaddr);
        if (clientsInQueue == MAXCLIENTS)
        {
            cout << "Server is busy with existing clients, new clients cannot be handled" << endl;
        }
        fflush(stdout);
        int clnSockFd = accept(sockFd, NULL, &clnAddrLen);
        if (clnSockFd > ZERO)
        {
            if (clientsInQueue < MAXCLIENTS)
            {
                clientSockFd[clientsInQueue++] = clnSockFd;
                const char *buffer = "Connection setup is successful";
                int len = strlen(buffer);
                send(clnSockFd, buffer, len, 0);
            }
        }
    }
    else
    {
        // Got a new message from the existing client
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
    int retVal;

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

    // // Set the socket as "non-blocking" socket.
    // u_long optVal = 1;
    // // optVal = 0 means "blocking" (default)
    // // optVal = 1 means "non-blocking"
    // retVal = ioctlsocket(sockFd, FIONBIO, &optVal);
    // if (retVal == ZERO)
    // {
    //     cout << "Socket is now in non-blocking mode" << endl;
    // }
    // else
    // {
    //     cout << "Error: Setting socket to non-blocking mode failed" << endl;
    // }

    // set the socket options() before the bind() call
    // int optVal = 0;
    // int optLen = sizeof(optVal);
    // retVal = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, (const char *)optVal, optLen);

    // if (retVal != ZERO)
    // {
    //     cout << "The setsockopt() call successfully set SO_REUSEADDR option" << endl;
    // }
    // else
    // {
    //     cout << "Error: The setsockopt() call failed for SO_REUSEADDR option";
    // }

    // Step 3: Binding the socket to the local port
    retVal = bind(sockFd, (sockaddr *)&srv, sizeof(sockaddr));

    if (retVal < ZERO)
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
    retVal = listen(sockFd, BACKLOG);

    if (retVal < ZERO)
    {
        cout << "Error: Failed in listening to local port" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Started listening to local port successfully" << endl;
    }

    maxFd = sockFd;

    // Step 5: Keep listening to new incoming request and start serving the requests
    while (1)
    {
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

        // cout << "Before select() call, fd_count: " << fr.fd_count << endl;
        struct timeval tval;
        tval.tv_sec = TIMEVALSECS;
        tval.tv_usec = TIMEVALUSECS;

        // select() call unsets the FD_SETs
        int readySocketFd = select(maxFd + 1, &fr, &fw, &fe, &tval);

        // retVal contains the total number of socket handles that are ready and contained in the FD_SET structures
        if (readySocketFd > ZERO)
        {
            processRequest(sockFd);
            // break;
            // cout << "New client arrived" << endl;
            // if (FD_ISSET(sockFd, &fe))
            // {
            //     cout << "Some exception occured on the socket" << endl;
            // }
            // if (FD_ISSET(sockFd, &fw))
            // {
            //     cout << "Ready to write on socket" << endl;
            // }
            // if (FD_ISSET(sockFd, &fr))
            // {
            //     cout << "Ready to read on socket" << endl;
            // }
        }
        else if (readySocketFd == ZERO)
        {
            cout << "No client's request arrived on port: " << PORT << endl;
        }
        else
        {
            cout << "Error in selecting the client's request" << endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        // cout << "After select() call, fd_count: " << fr.fd_count << endl;
        readySocketFd = 0;
    }
}
