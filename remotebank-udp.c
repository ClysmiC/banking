#include "banking.h"
#include "md5.h"

bool authenticate();

/**
 * UDP banking client. Allows user to deposit, withdraw
 * and check balance. Must interact with a server-udp program.
 * Skeleton code taken from TCP/IP Sockets in C by Donahoo
 * and Calvert.
 *
 */
int main(int argc, char *argv[])
{
    // state = UNAUTHENTICATED;

    /**Ensure proper command line args**/
    if (argc != 6 && argc != 7)
    {
        dieWithError("Usage: remotebank-udp <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
    }

    if (argc == 7)
    {
        if(strcmp(argv[6], "-d") == 0)
        {
            debugMode = true;
        }
        else
        {
            dieWithError("\nUsage: remotebank-udp <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]\n");
        }
    }

    char *name = argv[2];
    char *pass = argv[3];

    if(strlen(name) > 30 || strlen(pass) > 30)
    {
        dieWithError("\nUsername/password must be max 30 characters.\n");
    }

    //ensure valid request type
    if(strcasecmp(argv[4], "deposit") && strcasecmp(argv[4], "withdraw") && strcasecmp(argv[4], "checkbal"))
    {
        dieWithError("\nTransaction type must be one of the following:\n\tdeposit\n\twithdraw\n\tcheckbal\n");
    }


    /**Get position of colon within first argument**/
    char *colon = strchr(argv[1], ':');
    int colonPosition = colon - argv[1];

    if(colonPosition > 15)
    {
        dieWithError("\nInvalid IP address. Please use dotted quad notation.\n");
    }


    /**Substring first argument into IP and Port**/

    /**IP**/
    char serverIp[colonPosition + 1]; //+1 for \0 character

    memcpy(serverIp, &argv[1][0], colonPosition);
    serverIp[colonPosition] = '\0';

    if(((int)inet_addr(serverIp)) < 0)
        dieWithError("\nInvalid IP address. Please use dotted quad notation.\n");

    /**PORT**/
    int portStrLength = strlen(argv[1]) - colonPosition;
    char argumentPortStr[portStrLength + 1];
    memcpy(argumentPortStr, &argv[1][colonPosition + 1], portStrLength);
    argumentPortStr[portStrLength - 1] = '\0';

    int serverPortAsInt = atoi(argumentPortStr); //parses to int
    if(serverPortAsInt > USHRT_MAX) {
        dieWithError("\nInvalid port number.\n");
    }

    unsigned short serverPort = (unsigned short)serverPortAsInt;

    debugPrintf("IP: %s, Port: %d\n", serverIp, serverPort);



    /**Create socket**/ 
    int commSocket;

    if((commSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        dieWithError("socket(...) failed");
    }
    debugPrintf("UDP socket created. File descriptor: %d\n", commSocket);


    /**Fill out address structure for server address**/
    socket_address serverAddress;

    memset(&serverAddress, 0, sizeof(serverAddress)); //zero out
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIp);
    serverAddress.sin_port = htons(serverPort);

    //response available      
    char inBuffer[68];
    char *challenge;

    /**Send challenge request and listen for response**/
    bool ackReceived = false;

    int attempts = 0;
    while(attempts < 10)
    {
        char req_challenge[64];
        *((int*)req_challenge) = REQ_CHALLENGE;
        int bytesSent;

        if((bytesSent = sendto(commSocket, &req_challenge, sizeof(req_challenge), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < sizeof(int))
        {
            debugPrintf("Sent unexpected number of bytes. Expected: %d - Sent: %d\n", sizeof(req_challenge), bytesSent);
            dieWithError("Dying.");
        }

        debugPrintf("Sent challenge request\n");
        attempts++;

        /**Wait for response or timeout**/
        fd_set fds;
        struct timeval timeout;
        int rc, result;

        /* Set time limit. */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        FD_ZERO(&fds);
        FD_SET(commSocket, &fds);

        int val = select(commSocket + 1, &fds, NULL, NULL, &timeout);
        if(val <= 0)
        {
            //failed
            continue;
        }

        //response available
        int responseBytes = recv(commSocket, inBuffer, sizeof(inBuffer), 0);

        if(responseBytes != sizeof(inBuffer))
        {
            printf("Expected: %d - Received: %d", sizeof(challenge), responseBytes);
            dieWithError("Received wrong number of bytes when receiving challenge.");
        }

        int responseType = *((int*)inBuffer);

        debugPrintf("Received message type: %d\n", responseType);

        //make sure it is type challenge
        if(responseType != CHALLENGE)
        {
            printf("Expected: %d - Actual: %d", CHALLENGE, responseType);
            dieWithError("Received wrong message type");
        }

        challenge = &inBuffer[sizeof(int)];
        debugPrintf("Challenge received: %s\n", &inBuffer[sizeof(int)]);
        ackReceived = true;
        break;
    }

    if(!ackReceived)
    {
        dieWithError("Did not receive challenge from server.");
    }

    attempts = 0;
    ackReceived = false;
    while(attempts < 10)
    {

    }
}