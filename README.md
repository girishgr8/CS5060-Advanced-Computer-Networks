<h2 align="center">:computer: Assignment 2 - Socket Programming (File Transfer) :file_folder:</h2>

---

## PROBLEM STATEMENT

1.  Part A

    a. Create a server (file server) program. It listens on a TCP socket. The server is meant to send specific file(s) (.txt/.png/.pdf) to its client depending on the file requested (file path).

    b. Create a client program which interacts with the file server on TCP. Client TCP connects to the file server to fetch a file given the file path in the server. Assume that a valid client knows the ip address, port number of the file server.

2.  Part B

    a. Extend the file server program in Part A to serve multiple clients in parallel. i.e. if two different clients ask for a file, the server shall satisfy both the clients in parallel without making any client wait. Maximum number of clients to honor can be set at the server.

    b. Create one server program as per #a above and many instances of valid clients mentioned in Part A and check the working of parallel file download from the server. Check for all possible errors for which the server may give error message/error code to the client. E.g., file doesn’t exist. Check the situation when the server is busy serving other clients and one client has to wait for the server to start serving it after serving other clients.

---

## STEPS TO FOLLOW

Clone the repository and following the below steps:

**To run Part (A) of the problem statement:**

-   Run server program first in a separate terminal, amd compile the server's program
-   Here to compile the code, pass "-lwsock32" as a parameter for compilation of cpp code on Windows OS
-   Run the compiled object file (i.e. "server.exe" file)

```bash
$ cd PartA
$ cd server
$ g++ -o server server.cpp -lwsock32
$ ./server
```

-   Now, run client program first in a separate terminal, amd compile the client's program
-   Here to compile the code, pass "-lwsock32" as a parameter for compilation of cpp code on Windows OS
-   Run the compiled object file (i.e. "client.exe" file)

```bash
$ cd PartA
$ cd client
$ g++ -o client client.cpp -lwsock32
$ ./client
```

**To run Part (B) of the problem statement:**

-   Perform the steps similar to what is mentioned for Part (A)
-   Run server as follows:

```bash
$ cd PartB
$ cd server
$ g++ -o server multiServer.cpp -lwsock32
$ ./server
```

-   Run client as follows:

```bash
$ cd PartB
$ cd client
$ g++ -o client multiClient.cpp -lwsock32
$ ./client
```

#### NOTE: In Part (A), the server is able to handle the file transfer requests for below file formats: TXT, PNG, PDF, GIF, MP4.

---

## HOW IT WORKS

> The basic idea is to create one single server and one client program which will communicate over an established socket connection and on this connection file transfer will occur.

### Part (A)

#### SERVER CODE LOGIC

1. **Setup the socket programming enviornment on Windows:**

    - **WSADATA** data structure contains info about the Windows' way of socket implementation
    - **WSAStartup()** function initialises the winsock library and performs all necessary actions to enable socket programming environment.
    - **MAKEWORD(2,2)** is passed in WSAStartup() call, to load the 2.2 version of Winsock DLL library on the system.

2. **socket() system call:**

    - This creates a socket which can be used for communication with other networks.
    - This call returns the socket descriptor (imagine this as socket id)
    - Pass **AF_INET** which will use IPv4 version, **SOCK_STREAM** and **IPROTO_TCP** will set the TCP socket stream (i.e. connection-oriented stream)

3. **bind() system call:**
    - In step 2, we have just created the socket, but the socket is not assigned any IP address or Port number which can be used by the client to connect.
    - bind() will bind the address information such as IP address, Address Family and Port number specified by the variable of type _sockaddr_in_ to the socket which is described by the socket descriptor returned in above socket() call.
4. **listen() system call:**
    - Using this, server will listen to new incoming connection requests, i.e. server specifies it is now ready to accept new clients.
    - We pass _backlog_ parameter in the call which defines the maximum length to which the queue of pending connections for given socket descriptor may grow.
    - If any new connection request arrives when the queue is full, the client will receive an error _ECONNREFUSED_
5. **processClientRequest() function:**

    - This function does the handling of the client's requests incoming to server.
    - Firstly, **accept()** call accepts the client connection requests.
    - If the **accept()** call was successful, then server will send "Connection setup successful" as an acknowledgment to the client using the **send()** call.
    - Server then receives the filename of the file which client wants using **recv()** call in a buffer.

6. **sendFileToClient() function:**
    - Firstly, server sends **"FEBGIN"** to mark the beginning of file transfer so that client adds whatever data comes to it from now will be written into file on client side.
    - In one go, server reads _no. of bytes = buffersize_ from the file and sends those many bytes to client. If the client requests a valid file from server (i.e. if the file exists on the server), then server opens the file in _"rb"_ mode and keeps on reading the data from file until file ptr reaches the end of file.

#### CLIENT CODE LOGIC

1. **Setup the socket programming enviornment on Windows:**

    - **WSADATA** data structure contains info about the Windows' way of socket implementation
    - **WSAStartup()** function initialises the winsock library and performs all necessary actions to enable socket programming environment.
    - **MAKEWORD(2,2)** is passed in WSAStartup() call, to load the 2.2 version of Winsock DLL library on the system.

2. **socket() system call:**
    - This creates a socket which can be used for communication with other networks.
    - This call returns the socket descriptor (imagine this as socket id)
    - Pass **AF_INET** which will use IPv4 version, **SOCK_STREAM** and **IPROTO_TCP** will set the TCP socket stream (i.e. connection-oriented stream)
3. **connect() system call:**

    - In step 2, we have just created the socket, but the socket is not assigned any IP address or Port number which can be used by the client to connect.
    - connect() will connect to the server's socket, using the address information such as IP address, Address Family and Port number on which server is listening as specified by the variable of type _sockaddr_in_ to the connect which is described by the socket descriptor returned in above socket() call.

4. **Send filename of required file to server:**

    - Read the filename required by the client to the server by reading filename from the stdin console.
    - Send this filename to server using send() call.

5. **writeDataToFile() function:**
    - Once "FBEGIN" flag is received in the buffer, start writing everything into the newly created file on client's side till "FEND" flag is received in buffer.
    - The file for writing on client side is opened in _"wb"_ mode.

---

## PLAGIARISM STATEMENT

<p> I certify that this assignment/report is my own work, based on my personal study and/or research and that I have acknowledged all material and sources used in its preparation, whether they be books, articles, reports, lecture notes, and any other kind of document, electronic or personal communication. I also certify that this assignment/report has not previously been submitted for assessment in any other course, except where specific permission has been granted from all course instructors involved, or at any other time in this course, and that I have not copied in part or whole or otherwise plagiarized the work of other students and/or persons. I
pledge to uphold the principles of honesty and responsibility at CSE@IITH. In addition, I understand my responsibility to report honor violations by other students if I become aware of it. </p>

**Name of the student: THATTE GIRISH MAKARAND** <br>
**Roll No: CS22MTECH11005**

<b> This Assignment was done a part of our Semester-1 course on Advanced Computer Networks during M.Tech at IIT Hyderabad. </b>

<h3 align="center"><b>Developed with ❤️ by <a href="https://github.com/girishgr8">Girish Thatte</a>.</b></h3>
