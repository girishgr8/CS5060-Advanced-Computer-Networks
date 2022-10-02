#include <iostream>
#include <winsock.h>
#include <string>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <limits>
#include <ctime>

// define some constants
#define BOOTPORT 9000
#define PORT 9005
#define ONE 1
#define ZERO 0
#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILURE -1
#define BACKLOG 5
#define FLAGS 0
#define SERVERIPADDR "127.0.0.1"
#define MAXBUFFERSIZE 2048
#define MAXFILESERVERS 4

using namespace std;

struct FileServer
{
    string servicename;
    string servicetype;
    string ipaddress;
    string portnumber;
    string serviceaccesstoken;
};

// create global variables
FileServer fileservers[MAXFILESERVERS];
int currSize = 0;
vector<pair<string, vector<string>>> listOfFiles;
struct sockaddr_in cln, bootstrapServer, vdoSrv, pdfSrv, imgSrv, txtSrv;

// this function returns invalid access token if the random value generated is <= 0.5
// otherwise it returns correct access token
string randomTokenizer(string accesstoken)
{
    srand((unsigned int)time(NULL));
    int random = rand();
    float value = (float)random / (float)INT16_MAX;
    if (value <= 0.5)
        accesstoken = "##########";
    return accesstoken;
}

// createSocketAndConnect() function creates TCP socket and connects to TCP socket of specific file server
int createSocketAndConnect(struct sockaddr_in srv, FileServer *ptr, string serverType)
{
    char buffer[MAXBUFFERSIZE] = {ZERO};
    int sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd < ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
        cout << "Socket opened successfully at client with socket descriptor id: " << sockFd << endl;

    // Initialise the particular fileserver's socket address structure with correct values like server's port number and server's ip address
    srv.sin_port = htons(stoi(ptr->portnumber));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(ptr->ipaddress.c_str());
    memset(&(srv.sin_zero), 0, 8);
    int srvLen = sizeof(srv);

    // connect to the fileserver's TCP socket
    int retCode = connect(sockFd, (struct sockaddr *)&srv, srvLen);
    if (retCode < ZERO)
    {
        cout << "Error: Failed in connecting to " << serverType << "FileServer " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        retCode = recv(sockFd, buffer, MAXBUFFERSIZE, FLAGS);
        cout << buffer << ". Connected to " << serverType << "FileServer !!" << endl;
    }
    return sockFd;
}

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

// parseMessage() function : parses the metadata message received from bootstrap server and store that in our custom data structure
void parseMessage(string json)
{
    fileservers[currSize].servicename = tokenizer(json);
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

// fillFileServerMetadata() puts the received metadata of file servers' into our custom data structure
void fillFileServerMetadata(char json[])
{
    char *ptr = strtok(json, "$");
    while (ptr != NULL)
    {
        parseMessage(ptr);
        ptr = strtok(NULL, "$");
    }
}

// printFileServerStructureData() function prints the file servers' metadata
void printFileServerStructureData()
{
    struct FileServer *ptr = fileservers;
    int i = 0;
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

void parseFileServerResponse(char response[], FileServer *ptr)
{
    vector<string> srvFileList;
    char *p = strtok(response, ",");
    while (p != NULL)
    {
        srvFileList.push_back(string(p));
        p = strtok(NULL, ",");
    }
    pair<string, vector<string>> map;
    map.first = ptr->servicetype;
    map.second = srvFileList;
    listOfFiles.push_back(map);
}

// writeToFile() function writes the data recieved of the requested file
// Once the client receives "FBEGIN" flag from server, start writing to the file till "FEND" flag is received
// It iteratively writes no. of bytes = buffersize at one go which are no. of bytes received from the client
// If we receieve "FEND" flag in the buffer, then it marks end of writing into the file.
int writeDataToFile(int sockFd, char buff[], const char filename[])
{
    // Once "FBEGIN" flag is received, start writing all the data to be received in the file
    int noOfBytesRcvd = 0;
    if (strcmp(buff, "FBEGIN") != ZERO)
    {
        cout << "Some error occurred !" << endl;
        return FAILURE;
    }
    FILE *fp = fopen(filename, "wb");
    char buffer[MAXBUFFERSIZE];

    int totalSizeRcvd = 0;
    while (TRUE)
    {
        memset(buffer, 0, MAXBUFFERSIZE);
        noOfBytesRcvd = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
        // if the client receives "FEND" flag in buffer, then it indicates the ending of file transfer
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

// requestFileNamesFromServer() function gets the list of files present on particular file server
void requestFileNamesFromServer(int sockFd, FileServer *ptr, char recvBuffer[])
{
    int retCode = 0;
    char sendBuffer[MAXBUFFERSIZE] = {0};
    memset(recvBuffer, ZERO, MAXBUFFERSIZE);
    string message = "GETLISTOFFILES," + randomTokenizer(ptr->serviceaccesstoken);
    strcpy(sendBuffer, message.c_str());
    // Get video file names from Video File Server
    retCode = send(sockFd, sendBuffer, strlen(sendBuffer), FLAGS);
    if (retCode <= ZERO)
    {
        cout << "Error: Failed in sending service access token to " << ptr->servicename << ". Error code: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    retCode = recv(sockFd, recvBuffer, MAXBUFFERSIZE, FLAGS);
    // if the access token sent was invalid, then file server will reject the request
    if (strcmp(recvBuffer, "Invalid Client") == ZERO)
        cout << "Error: Server rejected request. Could not fetch list of files from " << ptr->servicename << ". " << recvBuffer << endl;
    if (retCode <= ZERO)
    {
        cout << "Error: Failed in receiving list of files from " << ptr->servicename << ". Error code: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}

// requestAndGetFile() function requests for a specific file from file server
void requestAndGetFile(int sockFd, FileServer *ptr, string serverType)
{
    int retCode = 0;
    char sendBuffer[MAXBUFFERSIZE], recvBuffer[MAXBUFFERSIZE];
    for (int i = 0; i < listOfFiles.size(); i++)
    {
        pair<string, vector<string>> pr = listOfFiles[i];
        if (pr.first.compare(serverType) == ZERO)
        {
            memset(sendBuffer, ZERO, MAXBUFFERSIZE);
            memset(recvBuffer, ZERO, MAXBUFFERSIZE);
            string filename = pr.second[0];
            cout << "File Requested from " << ptr->servicename << ": " << filename << endl;
            strcpy(sendBuffer, filename.c_str());
            // send the filename of required file to the server
            retCode = send(sockFd, sendBuffer, strlen(sendBuffer), FLAGS);
            // if the server has the requested file, it will send "FBEGIN" flag to client to mark the start of the file transfer
            retCode = recv(sockFd, recvBuffer, MAXBUFFERSIZE, FLAGS);
            if (retCode <= ZERO)
            {
                cout << "Some error occured !! Error code: " << WSAGetLastError() << endl;
                WSACleanup();
                exit(EXIT_FAILURE);
            }
            // Now write data to the file on client's side
            retCode = writeDataToFile(sockFd, recvBuffer, filename.c_str());

            if (retCode == SUCCESS)
            {
                // The closesocket() call closes an existing/open socket.
                closesocket(sockFd);
                cout << "Closing the socket now !!" << endl;
                break;
            }
            else if (retCode == FAILURE)
            {
                cout << "Some error occured !!" << endl;
                closesocket(sockFd);
                cout << "Closing the socket now !!" << endl;
                break;
            }
            // If file does not exist then close the connection
            if (strcmp(recvBuffer, "File not exist") == ZERO)
            {
                cout << "Your requested file does not exist on server !" << endl;
                closesocket(sockFd);
                cout << "Closing the socket now !!" << endl;
                break;
            }
            retCode = send(sockFd, sendBuffer, strlen(sendBuffer), FLAGS);
            if (retCode <= ZERO)
            {
                cout << "Error" << endl;
            }
        }
    }
}

int main()
{
    int retCode;
    char buffer[MAXBUFFERSIZE] = {0};
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
    int sockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sockFd <= ZERO)
    {
        cout << "Error: the socket could not be opened" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Socket opened successfully at client with socket descriptor id: " << sockFd << endl;
    }

    // Initialise the bootstrapserver's socket address structure with correct values like server's port number and server's ip address
    bootstrapServer.sin_family = AF_INET;
    bootstrapServer.sin_port = htons(BOOTPORT);
    bootstrapServer.sin_addr.s_addr = inet_addr(SERVERIPADDR);
    memset(&(bootstrapServer.sin_zero), 0, 8);
    int bootstrapServerLen = sizeof(bootstrapServer);

    cout << "Getting metadata for File Servers !" << endl;
    strcpy(buffer, "GETLISTOFFILESERVERS");
    retCode = sendto(sockFd, buffer, strlen(buffer), FLAGS, (struct sockaddr *)&bootstrapServer, bootstrapServerLen);

    if (retCode <= ZERO)
        cout << "Some error occured while sending filename to Bootstrap server. Try re-sending the filename to Bootstrap Server" << endl;

    memset(buffer, 0, MAXBUFFERSIZE);

    // receive the list of fileserver's metadata from bootstrapserver
    retCode = recvfrom(sockFd, buffer, MAXBUFFERSIZE, FLAGS, (struct sockaddr *)&bootstrapServer, &bootstrapServerLen);
    if (retCode <= ZERO)
        cout << "Some error occured while receiving file server metatdata from Bootstrap server." << endl;

    // store the metadata received from file server into our custom data structure
    fillFileServerMetadata(buffer);

    // print the metadata received
    printFileServerStructureData();

    struct FileServer *ptr = fileservers;
    int i = 0;
    int sockFdVdo, sockFdPdf, sockFdTxt, sockFdImg;
    char sendBuffer[MAXBUFFERSIZE], recvBuffer[MAXBUFFERSIZE];

    // create TCP socket connection with each type of file server
    while ((i++) < currSize)
    {
        // VideoServer
        if (ptr->servicetype.compare("video") == ZERO)
            sockFdVdo = createSocketAndConnect(vdoSrv, ptr, "Video");
        // PDFServer
        if (ptr->servicetype.compare("pdf") == ZERO)
            sockFdPdf = createSocketAndConnect(pdfSrv, ptr, "PDF");
        // ImageServer
        if (ptr->servicetype.compare("image") == ZERO)
            sockFdImg = createSocketAndConnect(imgSrv, ptr, "Image");
        // TextServer
        if (ptr->servicetype.compare("text") == ZERO)
            sockFdTxt = createSocketAndConnect(txtSrv, ptr, "Text");
        ptr++;
    }

    i = 0;
    ptr = fileservers;

    // request for file names from server and get specific file from each one of them
    while ((i++) < currSize)
    {
        // clear the buffers
        memset(sendBuffer, ZERO, MAXBUFFERSIZE);
        memset(recvBuffer, ZERO, MAXBUFFERSIZE);
        memset(buffer, ZERO, MAXBUFFERSIZE);

        // VideoServer
        if (ptr->servicetype.compare("video") == ZERO)
        {
            // request the file names from video server
            requestFileNamesFromServer(sockFdVdo, ptr, recvBuffer);
            // store the list of file names received
            parseFileServerResponse(recvBuffer, ptr);
            // print the list of file names received from file server
            for (int i = 0; i < listOfFiles.size(); i++)
            {
                pair<string, vector<string>> pr = listOfFiles[i];
                cout << "List of " << pr.first << " files is/are: " << endl;
                for (int i = 0; i < pr.second.size(); i++)
                    cout << (i + 1) << ") " << pr.second[i] << endl;
            }
            // request for specific file and get the file from file server
            requestAndGetFile(sockFdVdo, ptr, "video");
        }

        // PDFServer
        if (ptr->servicetype.compare("pdf") == ZERO)
        {
            // request the file names from pdf server
            requestFileNamesFromServer(sockFdPdf, ptr, recvBuffer);
            // store the list of file names received
            parseFileServerResponse(recvBuffer, ptr);
            for (int i = 0; i < listOfFiles.size(); i++)
            {
                pair<string, vector<string>> pr = listOfFiles[i];
                cout << "List of " << pr.first << " files is/are: " << endl;
                for (int i = 0; i < pr.second.size(); i++)
                    cout << (i + 1) << ") " << pr.second[i] << endl;
            }
            requestAndGetFile(sockFdPdf, ptr, "pdf");
        }

        // ImageServer
        if (ptr->servicetype.compare("image") == ZERO)
        {
            // request the file names from image server
            requestFileNamesFromServer(sockFdImg, ptr, recvBuffer);
            // store the list of file names received
            parseFileServerResponse(recvBuffer, ptr);
            for (int i = 0; i < listOfFiles.size(); i++)
            {
                pair<string, vector<string>> pr = listOfFiles[i];
                cout << "List of " << pr.first << " files is/are: " << endl;
                for (int i = 0; i < pr.second.size(); i++)
                    cout << (i + 1) << ") " << pr.second[i] << endl;
            }
            requestAndGetFile(sockFdImg, ptr, "image");
        }

        // TextServer
        if (ptr->servicetype.compare("text") == ZERO)
        {
            // request the file names from text server
            requestFileNamesFromServer(sockFdTxt, ptr, recvBuffer);
            // store the list of file names received
            parseFileServerResponse(recvBuffer, ptr);
            for (int i = 0; i < listOfFiles.size(); i++)
            {
                pair<string, vector<string>> pr = listOfFiles[i];
                cout << "List of " << pr.first << " files is/are: " << endl;
                for (int i = 0; i < pr.second.size(); i++)
                    cout << (i + 1) << ") " << pr.second[i] << endl;
            }
            requestAndGetFile(sockFdTxt, ptr, "text");
        }

        ptr++;
    }
}