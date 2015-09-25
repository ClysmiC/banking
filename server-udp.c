#include "banking.h"
#include "md5.h"

#define NUM_USERS 5

void generateRandomString(char*, const int);

bank_user users[NUM_USERS];

client_state clients[10]; //client states
int clientNumber = 0; //indexes into clients

/**
 * UDP banking server. Allows user to deposit, withdraw
 * and check balance. Serves remotebank-udp programs.
 * Skeleton code taken from TCP/IP Sockets in C by Donahoo
 * and Calvert.
 *
 */
int main(int argc, char *argv[])
{
    /**Ensure proper command line args**/
    if (argc != 2 && argc != 3)
        dieWithError("Usage: server-udp <Port> [-d]");

    if (argc == 3)
    {
        if(strcmp(argv[2], "-d") == 0)
        {
            debugMode = true;
        }
        else
            dieWithError("Usage: server-udp <Port> [-d]");
    }


    int portAsInt = atoi(argv[1]);
    if(portAsInt > USHRT_MAX)
        dieWithError("Invalid port number.");

    unsigned short port = (unsigned short)portAsInt;
    debugPrintf("Port number: %d\n", port);

    initUsers(users);

    /**Create socket**/
    int serverSocket;

    if((serverSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("socket(...) failed");

    debugPrintf("UDP socket created. File descriptor: %d\n", serverSocket);

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

    while(true)
    {
        char inBuffer[64];
        int inMsgType;
        int inHash;
        int inTransType;
        double inAmount;
        int inTransId;
        int inUsername;

        char outBuffer[68];

        socket_address clientAddress;
        int clientAddressLength = sizeof(clientAddress);

        debugPrintf("Waiting for message . . .\n");

        //receive incoming messages
        recvfrom(serverSocket, inBuffer, sizeof(inBuffer), 0, (struct sockaddr*)&clientAddress, &clientAddressLength);

        //extract essage type
        inMsgType = *((int*)inBuffer);

        debugPrintf("Received from %s:%d\n", inet_ntoa(clientAddress.sin_addr), serverAddress.sin_port);
        debugPrintf("Message type: %d\n", inMsgType);

        //perform actions based on message type
        if(inMsgType == REQ_CHALLENGE)
        {
            /**send challenge**/
            char randomString[CHALLENGE_SIZE + 1];
            generateRandomString(randomString, CHALLENGE_SIZE);
            randomString[CHALLENGE_SIZE] = '\0';

            *((int*)&outBuffer[0]) = CHALLENGE;
            memcpy(&outBuffer[sizeof(int)], randomString, CHALLENGE_SIZE);

            debugPrintf("Sending challenge: %s\n", randomString);

            sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
        }
    }
}

void generateRandomString(char *s, const int len)
{
   time_t t;
   
   /* Intializes random number generator */
   srand((unsigned) time(&t));

    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}