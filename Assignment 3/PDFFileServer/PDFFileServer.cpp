#include <iostream>
#include <winsock.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <tchar.h>
#include <filesystem>
#include <fstream>
#include <vector>
#define BOOTPORT 9000
#define PORT 9002
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
#define ACCESSTOKEN "pdfservertoken"
#define CHARSIZE sizeof(char)
#define FILEEXTENSION "pdf"
#define SERVERIPADDR "127.0.0.1"
using namespace std;

struct sockaddr_in srv, bootSrv;
FD_SET fr, fw, fe;
int maxFd;
int clientsInQueue = 0;

int clientSockFd[MAXCLIENTS];

vector<string> readDirectory(string name)
{
    vector<string> fileList;
    string pattern(name);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(_T(pattern).c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            fileList.push_back(data.cFileName);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
    return fileList;
}

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
        // cout << "Connection aborted. Closing the connection for client: " << clnSockFd << endl;
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
        char sendBuffer[MAXBUFFERSIZE] = {0};
        if (strncmp(buffer, "GETLISTOFFILES", 14) == ZERO)
        {
            string message = string(buffer);

            if (strcmp(message.substr(15, message.length()).c_str(), ACCESSTOKEN) != ZERO)
            {
                message = "Invalid Client";
                strcpy(sendBuffer, message.c_str());
                retCode = send(clnSockFd, sendBuffer, strlen(sendBuffer), FLAGS);
                if (retCode <= ZERO)
                {
                    cout << "Error in sending 'Invalid Client' as message to client: " << clnSockFd << endl;
                }
            }
            else
            {
                vector<string> fileList = readDirectory(".");
                string filenames = "";
                for (int i = 0; i < fileList.size(); i++)
                {
                    if (fileList[i].substr(fileList[i].find(".") + 1, fileList[i].length()).compare(FILEEXTENSION) == ZERO)
                    {
                        filenames += fileList[i];
                        if (i != fileList.size() - 1)
                            filenames += ",";
                    }
                }
                strcpy(sendBuffer, filenames.c_str());
                retCode = send(clnSockFd, sendBuffer, strlen(sendBuffer), FLAGS);
                if (retCode <= ZERO)
                {
                    cout << "Error in sending list of filenames to client: " << clnSockFd << endl;
                }
            }
        }
        else
        {
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

    // Step 2: Initialising the UDP socket
    // The socket() call by default creates a "blocking" socket.
    int sockFdUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // socket() system call returns -1 if there was a problem in opening a socket.
    if (sockFdUDP < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket opened successfully with socket descriptor id: " << sockFdUDP << endl;
    }

    // Initialising the environment variable for sockaddr structure
    bootSrv.sin_family = AF_INET;
    bootSrv.sin_port = htons(BOOTPORT); // i.e. specify the network byte order when specifying the port number
    // Method 1: The below command will set the server's IP address same as system's current IP address
    // srv.sin_addr.s_addr = INADDR_ANY;
    // Method 2: The below command will set the "Loopback IP address" of system in the long format in network byte order
    bootSrv.sin_addr.s_addr = inet_addr(SERVERIPADDR);
    // sin_zero is a char[8] array where we will set all the values to zero
    memset(&(bootSrv.sin_zero), 0, 8);

    string fileserver = "servicename:PDFServer,servicetype:pdf,ipaddress:127.0.0.1,portnumber:" + to_string(PORT) + ",serviceaccesstoken:" + ACCESSTOKEN;
    char buffer[MAXBUFFERSIZE] = {0};
    strcpy(buffer, fileserver.c_str());
    int bootSrvLen = sizeof(bootSrv);
    retCode = sendto(sockFdUDP, buffer, strlen(buffer), FLAGS, (struct sockaddr *)&bootSrv, bootSrvLen);
    if (retCode <= ZERO)
    {
        cout << "Some error occured !" << endl;
    }
    else
    {
        cout << "Done with adversting metadata !" << endl;
    }
    closesocket(sockFdUDP);

    // Initialising the TCP socket
    // The socket() call by default creates a "blocking" socket.
    int sockFdTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // socket() system call returns -1 if there was a problem in opening a socket.
    if (sockFdTCP < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket opened successfully with socket descriptor id: " << sockFdTCP << endl;
    }

    // Initialise the videofileserver's socket address structure with correct values like server's port number and server's ip address
    srv.sin_port = htons(PORT);
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(SERVERIPADDR);
    memset(&(srv.sin_zero), 0, 8);
    int srvLen = sizeof(srv);

    // Step 3: Binding the socket to the local port
    retCode = bind(sockFdTCP, (struct sockaddr *)&srv, srvLen);
    if (retCode < ZERO)
    {
        cout << "Error: Failed in binding the socket " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Step 4: Listening to the new requests from client (queueing the client's requests)

    // The 'backlog' parameter defines the maximum length to which the queue of pending connections for sockFd may grow.
    // If a connection request arrives when the queue is full,
    // the client may receive an error with an indication of ECONNREFUSED
    retCode = listen(sockFdTCP, BACKLOG);

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

    maxFd = sockFdTCP;

    // Step 5: Keep listening to new incoming request and start serving the existing requests in queue
    while (TRUE)
    {
        // clear the FD sets for read, write and exception socket descriptors
        FD_ZERO(&fr);
        FD_ZERO(&fw);
        FD_ZERO(&fe);

        FD_SET(sockFdTCP, &fr);
        FD_SET(sockFdTCP, &fe);

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
        int readySocketFd = select(maxFd + 1, &fr, &fw, &fe, NULL);
        // retCode contains the total number of socket handles that are ready and contained in the FD_SET structures
        if (readySocketFd > ZERO)
        {
            cout << "New client request arrived !" << endl;
            // New client request arrived
            processClientRequest(sockFdTCP);
        }
        else if (readySocketFd == ZERO)
        {
            cout << "No client's request arrived on port: " << PORT << endl;
        }
        readySocketFd = 0;
    }
}