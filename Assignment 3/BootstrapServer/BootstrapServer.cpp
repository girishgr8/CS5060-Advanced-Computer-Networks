#include <iostream>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#define PORT 9000
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
#define SERVERIPADDRESS "127.0.0.1"
#define MAXFILESERVERS 4
#define CHARSIZE sizeof(char)
using namespace std;

// FileServer is our custom data structure to store each file server's metadata
struct FileServer
{
    string servicename;
    string servicetype;
    string ipaddress;
    string portnumber;
    string serviceaccesstoken;
};

// global variables
FileServer fileservers[MAXFILESERVERS];
struct sockaddr_in srv, cln;
int currSize = ZERO;

// tokenizer() function returns the substring within two specific characters
string tokenizer(string message)
{
    int idx1 = message.find_first_of(":");
    int idx2 = message.find_first_of(",");
    string value = message.substr(idx1 + 1, idx2 - idx1 - 1);
    return value;
}

// index() function returns the index of characters after the first occurence of ','
int index(string message)
{
    return message.find_first_of(",") + 1;
}

// parseMessage() function : parses the metadata message received from file servers and store that in our custom data structure
void parseMessage(string json)
{
    fileservers[currSize].servicename = tokenizer(json);
    cout << "Message received from " << fileservers[currSize].servicename << endl;
    json = json.substr(index(json), json.length());
    fileservers[currSize].servicetype = tokenizer(json);
    json = json.substr(index(json), json.length());
    fileservers[currSize].ipaddress = tokenizer(json);
    json = json.substr(index(json), json.length());
    fileservers[currSize].portnumber = tokenizer(json);
    json = json.substr(index(json), json.length());
    fileservers[currSize].serviceaccesstoken = tokenizer(json);
    currSize++;
}

// printFileServerStructureData() function prints the file servers' metadata
void printFileServerStructureData()
{
    struct FileServer *ptr = fileservers;
    int i = ZERO;
    while ((i++) < currSize)
    {
        cout << "servicename:" << ptr->servicename << endl;
        cout << "servicetype:" << ptr->servicetype << endl;
        cout << "ipaddress:" << ptr->ipaddress << endl;
        cout << "portnumber:" << ptr->portnumber << endl;
        cout << "serviceaccesstoken:" << ptr->serviceaccesstoken << endl;
        cout << endl;
        ptr++;
    }
}

// prepareMetadata() function prepares the message to be sent to Client which includes file servers' metadata
string prepareMetadata()
{
    struct FileServer *ptr = fileservers;
    int i = ZERO;
    string metadata = "";
    while ((i++) < currSize)
    {
        metadata += "servicename:" + ptr->servicename + ",servicetype:" + ptr->servicetype + ",ipaddress:" + ptr->ipaddress + ",portnumber:" + ptr->portnumber + ",serviceaccesstoken:" + ptr->serviceaccesstoken + "$";
        ptr++;
    }
    return metadata;
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
        cout << "WSA Startup Initialised" << endl;

    // Step 2: Initialising the UDP socket
    // The socket() call by default creates a "blocking" socket.
    int sockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // socket() system call returns -1 if there was a problem in opening a socket.
    if (sockFd < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
        cout << "Socket opened successfully with socket descriptor id: " << sockFd << endl;

    // Initialising the environment variable for sockaddr structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT); // i.e. specify the network byte order when specifying the port number
    // Method 1: The below command will set the server's IP address same as system's current IP address
    // srv.sin_addr.s_addr = INADDR_ANY;
    // Method 2: The below command will set the "Loopback IP address" of system in the long format in network byte order
    srv.sin_addr.s_addr = inet_addr(SERVERIPADDRESS);
    // sin_zero is a char[8] array where we will set all the values to zero
    memset(&(srv.sin_zero), ZERO, 8);

    // Step 3: Binding the UDP socket to the local port
    retCode = bind(sockFd, (sockaddr *)&srv, sizeof(sockaddr));

    if (retCode < ZERO)
    {
        cout << "Error: Failed in binding socket to local port" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
        cout << "Socket binded to local port successfully" << endl;

    char buffer[MAXBUFFERSIZE] = {ZERO};

    // Step 4: Keep listening to new incoming request and start serving the existing requests in queue
    while (TRUE)
    {
        memset(&cln, ZERO, sizeof(cln));
        int clnLen = sizeof(cln);
        memset(buffer, ZERO, MAXBUFFERSIZE);

        // receive the request from client
        retCode = recvfrom(sockFd, buffer, MAXBUFFERSIZE, FLAGS, (struct sockaddr *)&cln, &clnLen);

        string metadata = string(buffer);

        // This request is from FileServer which advertises its metadata
        if (metadata.substr(0, 11).compare("servicename") == ZERO)
        {
            // store the metadata received from fileservers into our custom data structure
            parseMessage(metadata);
            // print the data stored in our custom data structure
            printFileServerStructureData();
        }
        // This request is from Client
        else if (strcmp(buffer, "GETLISTOFFILESERVERS") == ZERO)
        {
            string metadata = prepareMetadata();
            strcpy(buffer, metadata.c_str());

            // send the fileserver(s) metadata to client
            retCode = sendto(sockFd, buffer, strlen(buffer), FLAGS, (struct sockaddr *)&cln, clnLen);

            if (retCode <= ZERO)
                cout << "Error while sending fileserver(s) metadata to client" << endl;
        }
    }
}