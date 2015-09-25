============Project Info============
CS 3251-B
Programming Assignment 1
Due Date: 9/25/2015

============Author============
name: Andrew Smith
email: asmith379@gatech.edu

============Files submitted============
- banking.h 		- Header file containing #define'd constants and some common utility functions.
- md5.h 			- Implementation of md5 hashing function (copy/pasted from http://rosettacode.org/wiki/MD5#C)
- remotebank-tcp.c 	- TCP client for making withdraw/deposit requests to server.
- server-tcp.c 		- TCP server for handling withdraw/deposit requests from clients.
- remotebank-udp.c 	- UDP client for making withdraw/deposit requests to server.
- server-udp.c 		- UDP server for handling withdraw/deposit requests from clients.

============Compiling & Running============
A makefile is included. Running the command "make" should compile and output the following executables:

- remotebank-tcp.exe 	- Run using "./remotebank-tcp <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]"
						- Transaction type may be "deposit", "withdraw", or "checkbal". Amount is always required, but ignored when doing checkbal.
						- Fails if no server-tcp with the specified ip/port is running.

- server-tcp.exe 		- Run using "./server-tcp <Port> [-d]"
						- Runs indefinitely, handling one client at a time.

These programs were developed and tested in Windows 10, using Cygwin to simulate a Unix-like environment. The CoC computers may not have Cygwin installed,
so they should be tested on a Unix computer. If they fail to run successfully in that environment, please try test them in a Windows environment using Cygwin.

============Application Protocol============
- TCP Application
The TCP application protocol is as follows:
On server start, the server initializes 5 users into memory (see initUsers(...) in banking.h).
The act of the client connecting to the server initiates a 64-character alpha-numeric challenge string (valid for 10s) being sent from the server.
Upon receiving this challenge, the client computes the md5 hash of the following string: <name in lowercase> + <password> + <challenge>.
(Note: the username is invariably stored in all lower-case on the server side, so in order to acheive case-insensitive usernames, it must be converted to
lowercase before being hashed). The client then sends the following 3 things to the server, in this order: length of username (4 bytes), username
converted to lowercase (1-30 bytes), md5 hash (4 bytes). (Note: username and password are limited to 30 characters max). The server needs the
username to compute the hash on its end, however it does not know how long the username is. This is why the length is received first--
it is a known number of bytes (4), so it can be determined how many bytes to receive for the name. It then also receives the hash, which is a known size.
The server then looks up the username it received. If it cannot find the user name, it sends an AUTH_FAIL message to the client and closes
the socket. If it finds the user name, it computes the same md5(name + password + challenge) and compares it to the hash it received.
If they do not match, it sends an AUTH_FAIL message to the client and closes the socket. If they match, it sends an AUTH_SUCCESS and keeps
the socket open, waiting for a request to be sent. The client displays a failure message if an AUTH_FAIL is received, and closes the socket and exits.
If it receives an AUTH_SUCCESS message, it sends the following 2 things to the client: the request type (deposit, withdraw, or checkbal), and the
request amount (a double). If the client tries to deposit or withdraw a non-positive amount, the server sends a RESPONSE_INVALID_REQUEST message.
If the client tries to withdraw more money than is in their account, the server sends a RESPONSE_INSUFFICIENT_FUNDS message. Otherwise, it performs
the withdrawal/deposit (or does nothing in the case of checkbal) and returns a RESPONSE_SUCCESS message. Regardless of the response message, it also
sends the updated balance to the client immediately afterwards, and then closes the connection. The client receives the response and updated balance,
displays it to the user, then closes the socket and exits.

============Known Bugs/Limitations============
- Protocol assumes that both hosts agree on the format of data (primarily endianness and the size of primitive types).
- Protocol does not use time-outs. As long as both sides play nicely and follow the protocol and do not randomly disconnect this does not present a problem.

============Attributions============
- Much of the code's structure was inspired by the TCP and UDP client/server examples in TCP/IP Sockets in C by Kenneth Calvert and Michael Donahoo
- The md5 hash function was taken directly from rosettacode. http://rosettacode.org/wiki/MD5#C 