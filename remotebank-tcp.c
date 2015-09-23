#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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
	

	if (argc != 6 && argc != 7)
	{
		dieWithError("Usage: remotebank-tcp.c <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
	}

	if (argc == 7 && strcmp(argv[6], "-d") == 0)
	{
		debugMode = true;
	}

	int socket;

	socket_address serverAddress;

	char *colon = strchr(argv[1], ':');
	int colonPosition = colon - argv[1];

	if(colonPosition > 15)
	{
		dieWithError("Invalid IP address. Please use dotted quad notation.");
	}

	char serverIp[colonPosition + 1]; //+1 for \0 character

	debugPrintf("%s\n", argv[1]);
	debugPrintf("Colon position: %d\n", colonPosition);

	//substring first argument into IP and Port
	/**IP**/
	memcpy(serverIp, &argv[1][0], colonPosition);
	serverIp[colonPosition] = 0;

	/**PORT**/
	int portStrLength = strlen(argv[1]) - colonPosition;

	char argumentPortStr[portStrLength + 1];
	memcpy(argumentPortStr, &argv[1][colonPosition + 1], portStrLength);
	argumentPortStr[portStrLength - 1] = '\0';

	unsigned short serverPort = atoi(argumentPortStr);

	debugPrintf("IP: %s, Port: %d", serverIp, serverPort);
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
}