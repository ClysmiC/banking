#include "banking.h"
#include "md5.h"



void authenticate(int socket, char *user, char *pass);


/**
 * TCP banking client. Allows user to deposit, withdraw
 * and check balance. Must interact with a server-tcp program.
 * Skeleton code taken from TCP/IP Sockets in C by Donahoo
 * and Calvert.
 *
 */
int main(int argc, char *argv[])
{
    /**Ensure proper command line args**/
    if (argc != 6 && argc != 7)
    {
        dieWithError("Usage: remotebank-tcp.c <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
    }

    if (argc == 7)
    {
        if(strcmp(argv[6], "-d") == 0)
        {
            debugMode = true;
        }
        else
        {
            dieWithError("Usage: remotebank-tcp.c <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
        }
    }

    char *name = argv[2];
    char *pass = argv[3];

    if(strlen(name) > 30 || strlen(pass) > 30)
    {
        dieWithError("Username/password must be max 30 characters.");
    }


    /**Get position of colon within first argument**/
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
    int commSocket;

    if((commSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        dieWithError("socket(...) failed");
    }
    debugPrintf("TCP socket created. File descriptor: %d\n", commSocket);

    /**Fill out address structure for server address**/
    socket_address serverAddress;

    memset(&serverAddress, 0, sizeof(serverAddress)); //zero out
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIp);
    serverAddress.sin_port = htons(serverPort);

    debugPrintf("Connecting to %s:%d . . .\n", serverIp, serverPort);

    //establish connection
    if(connect(commSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        dieWithError("connect(...) failed.");
    }

    debugPrintf("Connection successful\n");

    socket_address clientAddress;
    int address_size = sizeof(clientAddress);
    getsockname(commSocket, (struct sockaddr *)&clientAddress, &address_size);

    debugPrintf("Client is on %s:%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

    authenticate(commSocket, argv[2], argv[3]);
}

void authenticate(int commSocket, char *user, char *pass)
{
    /**Receive challenge**/
    int totalBytes = 0;
    char challenge_buffer[CHALLENGE_SIZE + 1];

    debugPrintf("Begin receiving challenge\n");

    while(totalBytes < CHALLENGE_SIZE)
    {
        int received;
        if((received = recv(commSocket, &challenge_buffer[totalBytes], CHALLENGE_SIZE - totalBytes, 0)) <= 0)
            dieWithError("recv(...) failed.");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    challenge_buffer[CHALLENGE_SIZE] = '\0';
    debugPrintf("Challenge received: %s\n", challenge_buffer);

    /**force username to all lower*/
    for(int i = 0; i < strlen(user); i++)
    {
        user[i] = tolower(user[i]);
    }


    /**Hash user + pass + challenge**/
    int preHashSize = strlen(user) + strlen(pass) + CHALLENGE_SIZE + 1;
    char preHash[preHashSize];
    memcpy(preHash, user, strlen(user));
    memcpy(&preHash[strlen(user)], pass, strlen(pass));
    memcpy(&preHash[strlen(user) + strlen(pass)], challenge_buffer, CHALLENGE_SIZE);
    preHash[preHashSize - 1] = '\0';

    debugPrintf("Pre-hashed string: %s\n", preHash);

    unsigned int result = *md5(preHash, strlen(preHash));

    debugPrintf("Hashed result: %#x", result);

    /**
        Send length of username to server, so it knows how long
        to recv when reading username.
        Then send the username itself.
        Then send hashed result (fixed length) to server
    **/
    int usernameLength = strlen(user);
    send(commSocket, &usernameLength, sizeof(int), 0);
    send(commSocket, user, usernameLength, 0);
    send(commSocket, &result, sizeof(result), 0);
}