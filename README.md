# FTPserver

FTPserver is a simple command line tool that implements a File Transfer Protocol. FTPserver allows a client to interact with a directory using TCP socket connections.
Refer to **[RFC 959]** for more information on FTP

Implemented commands:
 
* NLST - lists directory names and file names in current working directory
* RETR - to retrieve files 
* PASV - start a data connection separate from the control connection
* USER - username must be **ANON**
* TYPE - only supports **ASCII** and **Image** types
* CWD  - can change working directory within root directory
* CDUP - can change to parent directory, but cannot go above the root directory 
* MODE - only supports **Stream** mode
* STRU - only supports **File** mode


[RFC 959]: https://www.w3.org/Protocols/rfc959/

# How to run
macOS: 
<details><summary><b>Show instructions</b></summary>
1. Open Terminal and run Makefile with port number between 1024 and 65535, inclusive.
 
```sh
make run <port #>
```

2. Open another Terminal window for client and connect using **netcat** or equivalent 

```sh
netcat <server address> <port #>
```

3. In client window, enter ANON as username. There is no password support for this version of FTP
```sh
user ANON
```

4. In client window, start passive mode
```sh
PASV
```
The FTPserver will return a new server address and port number in the form s1,s2,s3,s4,p1,p2

5. Open another Terminal window for file connection and connect using **netcat** and the server and port from step 4.

```sh
netcat <s1.s2.s3.s4> <new port>
```
<new port> is calculated using **p1 * 256 + p2**

6. In the client window, enter file transfer command for a test file
```sh
RETR test.txt
```

7. In the data window, the file contents will be printed on screen

</details>

Linux:

<details><summary><b>Show instructions</b></summary>
1. Open command line and run Makefile with port number between 1024 and 65535, inclusive.
 
```sh
make run <port #>
```

2. Open another command line window for client and connect using **ftp** or equivalent 

```sh
ftp <server address> <port #>
```

3. In client window, enter ANON as username when prompted. There is no password support for this version of FTP
```sh
user ANON
```

4. In the client window, enter file transfer command for a test file
```sh
get test.txt test-recieved.txt
```
</details>

# How it works

FTPserver listens on a welcome socket for new incoming connections.

Client opens a new socket connection and connect to the same server address and port number as FTPserver. 

FTPserver sends control messages and reply codes based on client commands.

# Debugging tips
* If transferred file is corrupted, try switching the mode to Image for pdf and .png files.

* Linux's **ftp** client program handles [file transfer commands]. Client can also use **netcat** to connect to server and follow the same instructions as macOS.

* Cannot change working directory to above the root directory, for security.

[file transfer commands]: https://www.serv-u.com/features/file-transfer-protocol-server-linux/commands
