#include <iostream>
#include <winsock.h>
#include <string>
#include <algorithm>
#define PORT 9001
#define ONE 1
#define ZERO 0
#define SUCCESS 1
#define FAILURE -1
#define BACKLOG 5
#define SERVERIPADDR "127.0.0.1"
#define MAXBUFFERSIZE 200000

using namespace std;

struct sockaddr_in cln;

FD_SET fr, fw, fe;
int maxFd;

// void writefile(int sockfd, FILE *fp)
// {
//     ssize_t n;
//     char buff[MAX_LINE] = {0};
//     while ((n = recv(sockfd, buff, MAX_LINE, 0)) > 0)
//     {
//         total += n;
//         if (n == -1)
//         {
//             perror("Receive File Error");
//             exit(1);
//         }

//         if (fwrite(buff, sizeof(char), n, fp) != n)
//         {
//             perror("Write File Error");
//             exit(1);
//         }
//         memset(buff, 0, MAX_LINE);
//     }
// }

int writeFile(int sockFd, char *filename)
{
    // cout << "Inside write file" << endl;

    // string fileData = "";
    // while (1)
    // {
    //     memset(&buffer, 0, MAXBUFFERSIZE);
    //     int retVal = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
    //     int bufferLen = strlen(buffer);
    //     if (retVal == ZERO)
    //     {
    //         cout << "Server closed the connection gracefully" << endl;
    //     }
    //     // recv() call returns number of bytes received
    //     else if (retVal > ZERO)
    //     {
    //         if (bufferLen == ZERO)
    //         {
    //             cout << "" << endl;
    //             break;
    //         }
    //         cout << "Server replied: " << buffer << endl;
    //         fileData += buffer;
    //     }
    // }

    int retVal;
    FILE *fp;
    char buffer[MAXBUFFERSIZE];

    fp = fopen("data.txt", "wb");
    int i = 0;
    while (1)
    {
        memset(&buffer, 0, MAXBUFFERSIZE);
        retVal = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
        // Sleep(500);
        cout << "buffer = " << buffer << ", size at i = " << i << "\t" << strlen(buffer) << "\tretVal = " << retVal << endl;
        // cout << "Iteration " << (i++) << ", size = " << strlen(buffer) << ", retVal = " << retVal << endl;
        if (strcmp(buffer, "FEND") == ZERO)
        {
            cout << "Requested file received" << endl;
            break;
            // fclose(fp);
            // return SUCCESS;
        }
        // fprintf(fp, "%s", buffer);
        int writtenBytes = fwrite(buffer, ONE, strlen(buffer), fp);
        cout << "writtenBytes = " << writtenBytes << endl;
        memset(&buffer, 0, MAXBUFFERSIZE);
        buffer[0] = 'A';
        buffer[1] = 'C';
        buffer[2] = 'K';
        retVal = send(sockFd, buffer, strlen(buffer), 0);
    }
    fclose(fp);
    return SUCCESS;
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
        cout << "Socket opened successfully at client with socket descriptor id: " << sockFd << endl;
    }

    cln.sin_family = AF_INET;
    cln.sin_port = htons(PORT);
    cln.sin_addr.s_addr = inet_addr(SERVERIPADDR);

    memset(&(cln.sin_zero), 0, 8);

    retVal = connect(sockFd, (struct sockaddr *)&cln, sizeof(cln));

    if (retVal < ZERO)
    {
        cout << "Error: Failed in connecting to server" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Client connected to the server successfully" << endl;
        char buffer[MAXBUFFERSIZE];
        memset(&buffer, '\0', MAXBUFFERSIZE);
        recv(sockFd, buffer, MAXBUFFERSIZE, 0);
        cout << "Hit ENTER to continue:" << endl;
        getchar();
        cout << "Server replied: " << buffer << endl;

        while (1)
        {
            cout << "Hit ENTER to continue:" << endl;
            getchar();
            cout << "Enter filepath for the request object: " << endl;
            fgets(buffer, MAXBUFFERSIZE, stdin);
            int bufferLen = strlen(buffer);

            send(sockFd, buffer, strlen(buffer), 0);
            memset(&buffer, 0, MAXBUFFERSIZE);
            int retVal = recv(sockFd, buffer, MAXBUFFERSIZE, 0);
            cout << "Here " << retVal << " " << buffer << endl;
            if (retVal < ZERO)
            {
                cout << "Some error occured" << endl;
                exit(1);
            }
            if (strcmp(buffer, "FBEGIN") == ZERO)
            {
                cout << "Yes lets start file transfer" << endl;
                retVal = writeFile(sockFd, buffer);
            }
            // retVal = writeFile(sockFd, buffer);
            cout << "retVal " << retVal << endl;
            if (retVal == SUCCESS)
            {
                // The closesocket() call closes an existing/open socket.
                closesocket(sockFd);
                cout << "closesocket() call done";
                break;
            }
        }
    }
}
