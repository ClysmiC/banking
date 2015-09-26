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

- UDP Application
On server start, the server initializes 5 users into memory (see initUsers(...) in banking.h).
The client always sends 64 byte messages to the server, and the server always sends 68 byte messages to the client. The first 4 bytes of every
message indicate what type of message it is. The client can send the following two message types: "Request Challenge" and "Anwser + Request Transaction".
The server can send the following two message types: "Challenge" and "Transaction Response". The client must initiate the communication by sending
a "Request Challenge" message. The first 4 bytes indicate the message type of REQ_CHALLENGE, and the remaining 60 bytes are unused. When the server 
receives a REQ_CHALLENGE message, it generates a random 64 character alphanumeric string. It stores the requesting server IP and port, as well as the
challenge (valid for 10s) being assigned to them in its list of "active connections". It then sends a CHALLENGE packet, whose first 4 bytes are the 
message type, and remaining 64 bytes are the challenge string. After the client sends a REQ_CHALLENGE, it listens for a CHALLENGE message. If it does not 
hear one within 2 seconds, it assumes a packet was lost, and resends the REQ_CHALLENGE. If it attempts this 10 times and still receives no message, it 
exits. After receiving CHALLENGE, The client then computes the md5 hash of (<name in lowercase> + <password> + <challenge>), and sends a packet contaning 
its answer to the challenge as well as its transaction request. The packet is layed out as follows. Bytes 0-3: msg type - 4-7: md5 hash; 8-11: transaction 
type; 12-19: transaction amount; 20-23 transaction id; 24-63 username. All of the fields are self-explanatory, besides the transaction id. The id is a 
randomly generated number that the server checks to make sure the client didn't think a packet was lost and send a duplicate request. The server responds 
to REQ_TRANSACTIONS with RESPONSE messages. These messages have 4 bytes for the type, 4 for the response code, and 8 bytes for the remaining balance. The 
remaining 54 bytes go unused. When the server gets a REQ_TRANSACTION message, it first looks up the challenge string for this IP/port, and looks 
up the username from the message. If neither of these are found, it sends a RESPONSE with code AUTH_FAIL. It then calculates the md5 on the server side,
sending a RESPONSE with code AUTH_FAIL if the received and calculated hashes don't match. It then checks the transaction id against the last successful
transaction id for this IP/port. If they match, it does nothing, but still sends a RESPONSE with code SUCCESS, and the unchanged balance. It does this
because it assumes the duplicate transaction was due to the client not receiving the response the first time. If the client tries to deposit or withdraw a 
non-positive amount, the server sends a RESPONE with code INVALID_REQUEST message. If the client tries to withdraw more money than is in their account, 
the server sends a RESPONSE message with code INSUFFICIENT_FUNDS. Otherwise, it performs the withdrawal/deposit (or does nothing in the case of checkbal) 
and returns a RESPONSE with code SUCCESS. In the case of successful withdrawals/deposits, it also records the transaction id for this Server/IP to prevent 
a duplicate request. All RESPONSE messages (besides those with code AUTH_FAIL) also contain the updated balance to the client immediately 
after the transaction. After the client sends a REQ_TRANSACTION, it listens for a RESPONSE message. If it does not hear one within 2 seconds,
it assumes a packet was lost, and resends the REQ_TRANSACTION (with the same transaction id). If it attempts this 10 times and still receives no message, 
it exits, telling the user that the transaction MAY have gone through, and they should use "checkbal" before attempting any more transactions to find out. 
If it gets a response, it displays to the user if their credentials were wrong or the transaction failed or succeeded, as described by the response code, 
as well as the updated balance. The client then closes the socket to the server and exits.

============Known Bugs/Limitations============
- Protocol assumes that both hosts agree on the format of data (primarily endianness and the size of primitive types).
- TCP Protocol does not use time-outs. As long as both sides play nicely and follow the protocol and do not randomly disconnect this does not present a problem

============Attributions============
- Much of the code's structure was inspired by the TCP and UDP client/server examples in TCP/IP Sockets in C by Kenneth Calvert and Michael Donahoo
- The md5 hash function was taken directly from rosettacode. http://rosettacode.org/wiki/MD5#C 