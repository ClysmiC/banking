#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>

#include "banking.h"

typedef struct sockaddr_in socket_address;

void dieWithError(char *error);
void debugPrintf(char *fstring, ...);

bool debugMode = false;

int main(int argc, char *argv[])
{
	/**Ensure proper # of command line args**/
	if (argc != 6 && argc != 7)
	{
		dieWithError("Usage: remotebank-tcp.c <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
	}

	if (argc == 7 && strcmp(argv[6], "-d") == 0)
	{
		debugMode = true;
	}


	/**Get position of colon within first argument**/
	int clientSocket;
	socket_address serverAddress;

	char *colon = strchr(argv[1], ':');
	int colonPosition = colon - argv[1];

	if(colonPosition > 15)
	{
		dieWithError("Invalid IP address. Please use dotted quad notation.");
	}


	/**Substring first argument into IP and Port**/

	/**IP**/
	char serverIp[colonPosition + 1]; //+1 for \0 character

	memcpy(serverIp, &argv[1][0], colonPosition);
	serverIp[colonPosition] = '\0';

	if(((int)inet_addr(serverIp)) < 0)
		dieWithError("Invalid IP address. Please use dotted quad notation.");

	/**PORT**/
	int portStrLength = strlen(argv[1]) - colonPosition;
	char argumentPortStr[portStrLength + 1];
	memcpy(argumentPortStr, &argv[1][colonPosition + 1], portStrLength);
	argumentPortStr[portStrLength - 1] = '\0';

	int serverPortAsInt = atoi(argumentPortStr); //parses to int
	if(serverPortAsInt > USHRT_MAX) {
		dieWithError("Invalid port number.");
	}

	unsigned short serverPort = (unsigned short)serverPortAsInt;

	debugPrintf("IP: %s, Port: %d\n", serverIp, serverPort);



	/**Create socket**/

	//TPPROTO_TCP for third argument not working on my windows machine. Using 0 (default) instead.
	if((clientSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		dieWithError("socket(...) failed");
	}
	debugPrintf("TCP socket created. File descriptor: %d\n", clientSocket);

	//construct server address structure
	memset(&serverAddress, 0, sizeof(serverAddress)); //zero out
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIp);
	serverAddress.sin_port = htons(serverPort);

	debugPrintf("Connecting to %s:%d . . .\n", serverIp, serverPort);

	//establish connection
	if(connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	{
		dieWithError("connect(...) failed.");
	}

	debugPrintf("Connection successful");
}

void dieWithError(char *error)
{
	fflush(stdout);
	printf("%s\n", error);
	fflush(stdout);
	exit(1);
}

void debugPrintf(char *fstring, ...)
{
	if(!debugMode)
		return;

	va_list args;
    va_start(args, fstring);
    vprintf(fstring, args);
    va_end(args);
    fflush(stdout);
}