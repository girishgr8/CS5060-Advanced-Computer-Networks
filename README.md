<h1 align="center">:computer: Assignment 2 - Socket Programming (File Transfer) :file_folder:</h1>

---

# Steps to Follow

Clone the repository and do the following:

```bash
# Run the below command in separate terminal, and then compile the server's program (here to compile the code, pass "-lwsock32" as a parameter for compilation of cpp code on Windows OS)

$ cd server
$ g++ -o server server.cpp -lwsock32

# Run the compiled object file (i.e. "server.exe" file)
$ ./server

# Run the below command in separate terminal, and then compile the client's program (here to compile the code, pass "-lwsock32" as a parameter for compilation of cpp code on Windows OS)
$ cd client
$ g++ -o client client.cpp -lwsock32

# Run the compiled object file (i.e. "server.exe" file)
$ ./client
```
