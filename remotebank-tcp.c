#include "banking.h"
#include "md5.h"



bool authenticate(int socket, char *user, char *pass);


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
        dieWithError("Usage: remotebank-tcp <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]");
    }

    if (argc == 7)
    {
        if(strcmp(argv[6], "-d") == 0)
        {
            debugMode = true;
        }
        else
        {
            dieWithError("\nUsage: remotebank-tcp <Server IP>:<Port> <Username> <Password> <Transaction Type> <Transaction Amount> [-d]\n");
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

    /** AUTHENTICATE! **/
    if(!authenticate(commSocket, argv[2], argv[3]))
    {
        printf("Authentication failed\n");
        close(commSocket);
        exit(0);
    }

    debugPrintf("Authentication successful\n");

    /** Fill out transaction request struct **/
    int requestType;
    double requestAmount;

    if(strcasecmp(argv[4], "deposit") == 0)
    {
        requestType = TRANSACTION_DEPOSIT;
    }
    else if(strcasecmp(argv[4], "withdraw") == 0)
    {
        requestType = TRANSACTION_WITHDRAW;
    }
    else if(strcasecmp(argv[4], "checkbal") == 0)
    {
        requestType = TRANSACTION_CHECKBAL;
    }
    else
    {
        dieWithError("Internal error processing request type.");
    }

    requestAmount = atof(argv[5]);

    /**Send transaction request**/
    send(commSocket, &requestType, sizeof(requestType), 0);
    send(commSocket, &requestAmount, sizeof(requestAmount), 0);


    debugPrintf("Sent request. Type: %d -- Amount: %.2f\n", requestType, requestAmount);

    /**Receive transaction response**/
    int totalBytes = 0;
    int BYTES_TO_RECEIVE = sizeof(int) + sizeof(double);
    int response;
    double updatedBalance;
    char responseBuffer[BYTES_TO_RECEIVE];

    debugPrintf("Receiving transaction response . . .\n");

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &responseBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving transaction response");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    response = (*(int*)responseBuffer);
    updatedBalance = (*(double*)&responseBuffer[sizeof(int)]);

    /**PRINT RESULTS**/
    debugPrintf("Response: %d -- Balance: $%.2f\n", response, updatedBalance);

    printf("\nWelcome to online banking, %s\n", argv[2]);
    
    if(requestType == TRANSACTION_DEPOSIT)
    {
        if(response == RESPONSE_SUCCESS)
        {
            printf("Deposit of $%.2f successful. New balance: %.2f\n", requestAmount, updatedBalance);
        }
        else if(response == RESPONSE_INVALID_REQUEST)
        {
            printf("Invalid deposit request\n");
        }
    }
    else if(requestType == TRANSACTION_WITHDRAW)
    {
        if(response == RESPONSE_SUCCESS)
        {
            printf("Withdrawal of $%.2f successful. New balance: %.2f\n", requestAmount, updatedBalance);
        }
        else if(response == RESPONSE_INSUFFICIENT_FUNDS)
        {
            printf("Insufficient funds to withdraw $%.2f. Balance remains at $%.2f\n", requestAmount, updatedBalance);
        }
        else   
        {
            printf("Invalid withdraw request\n");
        }
    }
    else if(requestType == TRANSACTION_CHECKBAL)
    {
        if(response == RESPONSE_SUCCESS)
        {
            printf("Your balance is %.2f\n", updatedBalance);
        }
        else if(response == RESPONSE_INVALID_REQUEST)
        {
            printf("Invalid checkbal request\n");
        }
    }

    close(commSocket);
    exit(0);
}

bool authenticate(int commSocket, char *user, char *pass)
{
    /**Receive challenge**/
    int totalBytes = 0;
    char challenge_buffer[CHALLENGE_SIZE + 1];
    int BYTES_TO_RECEIVE = CHALLENGE_SIZE;

    debugPrintf("Begin receiving challenge\n");

    while(totalBytes < BYTES_TO_RECEIVE)
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

    debugPrintf("Hashed result: %#x\n", result);

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

    debugPrintf("Hash sent\nAwaiting authentication\n");

    /**Receive auth response**/
    totalBytes = 0;
    char authResponseBuffer[sizeof(int)];
    int authResponse;
    BYTES_TO_RECEIVE = sizeof(int);

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &authResponseBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) <= 0)
            dieWithError("recv(...) failed.");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    authResponse = (*(int*)authResponseBuffer);

    debugPrintf("AuthResponse: %d\n", authResponse);


    if(authResponse == AUTH_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}