#include <iostream>
#include <winsock.h>
#include <string>
#include <algorithm>

// define some constants
#define PORT 9001
#define ONE 1
#define ZERO 0
#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILURE -1
#define BACKLOG 5
#define SERVERIPADDR "127.0.0.1"
#define MAXBUFFERSIZE 2048

using namespace std;

// create global variables
struct sockaddr_in cln;

int writeToFile(int sockFd, char filename[])
{
    int noOfBytesRcvd;
    filename[strlen(filename) - 1] = '\0';
    FILE *fp = fopen(filename, "wb");
    char buffer[MAXBUFFERSIZE];
    int totalSizeRcvd = 0;
    while (TRUE)
    {
        memset(&buffer, 0, MAXBUFFERSIZE);
        noOfBytesRcvd = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
        if (strncmp(buffer, "FEND", 4) == ZERO)
        {
            cout << "Requested file received" << endl;
            break;
        }
        totalSizeRcvd += noOfBytesRcvd;
        int bytesWritten = fwrite(buffer, ONE, noOfBytesRcvd, fp);
    }
    fclose(fp);
    return SUCCESS;
}

int main()
{
    int retCode;

    // Step 1: To initialise the WSA variable
    // The WSAStartup() call initiates use of the Winsock DLL (Dynamic Link Library) by a process,
    // i.e. allows to specify version of the Winsock library one wants to use
    // The WSACleanup() call terminates use of the Winsock DLL (Dynamic Link Library)
    WSADATA ws;

    if (WSAStartup(MAKEWORD(2, 2), &ws) < ZERO)
    {
        cout << "WSAStartup() called failed" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "WSAStartup() call initialised" << endl;
    }

    // socket() system call returns -1 if there was some problem in opening socket.
    // otherwise it returns the socket descriptor of the created socket
    int sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockFd < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket opened successfully at client with socket descriptor id: " << sockFd << endl;
    }

    // Initialise the client's socket address structure with correct values like server's port number and server's ip address
    cln.sin_family = AF_INET;
    cln.sin_port = htons(PORT);
    cln.sin_addr.s_addr = inet_addr(SERVERIPADDR);
    memset(&(cln.sin_zero), 0, 8);

    // connect() call tries to build a connection between two sockets
    retCode = connect(sockFd, (struct sockaddr *)&cln, sizeof(cln));

    if (retCode < ZERO)
    {
        cout << "Error: Failed in connecting to server" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        // connection setup will be successful, then receive the data and
        // write to file until the server does not send "FEND" flag to mark end of file transfer
        char buffer[MAXBUFFERSIZE];
        memset(&buffer, '\0', MAXBUFFERSIZE);
        retCode = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
        if (strcmp(buffer, "Connection setup successful") != ZERO)
        {
            cout << "Some error occurred while connection setup !" << endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        cout << "You are connected to the server successfully" << endl;
        cout << "Server replied: " << buffer << endl;
        cout << "Hit ENTER to continue:" << endl;
        getchar();

        while (TRUE)
        {
            cout << "Enter filepath for the request object: " << endl;
            fgets(buffer, MAXBUFFERSIZE, stdin);
            int bufferLen = strlen(buffer);
            char filename[MAXBUFFERSIZE];
            strcpy(filename, buffer);

            // send the filename of required file to the server
            retCode = send(sockFd, buffer, strlen(buffer), 0);
            memset(&buffer, 0, MAXBUFFERSIZE);

            // if the server has the requested file, it will send "FBEGIN" flag to client to mark the start of the file transfer
            retCode = recv(sockFd, buffer, MAXBUFFERSIZE, 0);

            if (retCode < ZERO)
            {
                cout << "Some error occured: " << endl;
                WSACleanup();
                exit(EXIT_FAILURE);
            }
            // Once "FBEGIN" flag is received, start writing all the data to be received in the file.
            if (strcmp(buffer, "FBEGIN") == ZERO)
            {
                retCode = writeToFile(sockFd, filename);
                if (retCode == SUCCESS)
                {
                    // The closesocket() call closes an existing/open socket.
                    closesocket(sockFd);
                    cout << "Closing the socket now !!" << endl;
                    break;
                }
            }
            // if file does not exist then close the connection
            else if (strcmp(buffer, "File not exist") == ZERO)
            {
                cout << "Your requested file does not exist on server !" << endl;
                closesocket(sockFd);
                cout << "Closing the socket now !!" << endl;
                break;
            }
        }
    }
}