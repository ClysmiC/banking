#include "banking.h"
#include "md5.h"

void generateRandomString(char*, const int);

void authenticate(int, bank_user*);

bank_user users[5];

/**
 * TCP banking server. Allows user to deposit, withdraw
 * and check balance. Serves remotebank-tcp programs.
 * Skeleton code taken from TCP/IP Sockets in C by Donahoo
 * and Calvert.
 *
 */
int main(int argc, char *argv[])
{
    /**Ensure proper command line args**/
    if (argc != 2 && argc != 3)
        dieWithError("Usage: server-tcp <Port>");

    if (argc == 3)
    {
        if(strcmp(argv[2], "-d") == 0)
        {
            debugMode = true;
        }
        else
            dieWithError("Usage: server-tcp <Port>");
    }


    int portAsInt = atoi(argv[1]);
    if(portAsInt > USHRT_MAX)
        dieWithError("Invalid port number.");

    unsigned short port = (unsigned short)portAsInt;
    debugPrintf("Port number: %d\n", port);

    initUsers(users);

    /**Create socket**/
    int serverSocket;

    if((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("socket(...) failed");

    debugPrintf("TCP socket created. File descriptor: %d\n", serverSocket);

    /**Fill out address structure for binding socket**/
    socket_address serverAddress;

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    /**Bind socket**/
    debugPrintf("Binding to port %d\n", port);
    if(bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        dieWithError("bind(...) failed.");

    debugPrintf("Bound.\n");

    /**Listen**/
    if(listen(serverSocket, 10) < 0)
        dieWithError("listen(...) failed.");

    debugPrintf("Listening.\n");
    printf("\nServer-tcp is bound and listening.\n");
    fflush(stdout);


    /**Process requests**/
    while(true)
    {
        int commSocket;
        socket_address clientAddress;
        socklen_t clientLength = sizeof(clientAddress);

        memset(&clientAddress, 0, sizeof(clientAddress));

        if((commSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength)) < 0)
            dieWithError("accept(...) failed.");

        debugPrintf("Handling client %s:%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);
        debugPrintf("Socket file descriptor: %d\n", commSocket);

        bank_user *authenticatedUser;
        authenticate(commSocket, authenticatedUser);

        if(authenticatedUser == NULL)
        {
            close(commSocket);
            debugPrintf("Closing client socket.\n");
        }       
    }
}

void authenticate(int commSocket, bank_user* user_out)
{
    //send challenge
    char randomString[CHALLENGE_SIZE];
    generateRandomString(randomString, CHALLENGE_SIZE);
    debugPrintf("Sending challenge: %s\n", randomString);

    send(commSocket, randomString, sizeof(randomString), 0);

    debugPrintf("Challenge sent.\n");

    while(true){}

    //recv response


    //compare md5 hash

    //if same, output corresponding user

    //else output null
    user_out = NULL;
}

void generateRandomString(char *s, const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}