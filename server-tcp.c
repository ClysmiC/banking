#include "banking.h"
#include "md5.h"

#define NUM_USERS 5

void generateRandomString(char*, const int);
void authenticate(int, bank_user**);
void handleRequest(int, bank_user*);

bank_user users[NUM_USERS];

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
        dieWithError("Usage: server-tcp <Port> [-d]");

    if (argc == 3)
    {
        if(strcmp(argv[2], "-d") == 0)
        {
            debugMode = true;
        }
        else
            dieWithError("Usage: server-tcp <Port> [-d]");
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

        /**Accept socket**/
        if((commSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength)) < 0)
            dieWithError("accept(...) failed.");

        debugPrintf("\nHandling client %s:%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);
        debugPrintf("Socket file descriptor: %d\n", commSocket);

        /**Authenticate user**/
        bank_user *authUser;
        authenticate(commSocket, &authUser);

        int authResponse;

        if(authUser == NULL)
        {
            /**Authentication fail**/
            printf("***Failed authentication attempt***\n");
            authResponse = AUTH_FAIL;
            send(commSocket, &authResponse, sizeof(authResponse), 0);

            close(commSocket);
            debugPrintf("Closing client socket.\n\nWaiting for next request\n\n");
            continue;
        }

        /**Authentication success**/
        /**send response packet**/
        debugPrintf("Authenticated as %s:%s\n", authUser->name, authUser->password);
        authResponse = AUTH_SUCCESS;
        send(commSocket, &authResponse, sizeof(authResponse), 0);


        handleRequest(commSocket, authUser);
        close(commSocket);
    }
}

void authenticate(int commSocket, bank_user **user_out)
{
    /**send challenge**/
    char randomString[CHALLENGE_SIZE + 1];
    generateRandomString(randomString, CHALLENGE_SIZE);
    randomString[CHALLENGE_SIZE] = '\0';

    debugPrintf("Sending challenge: %s\n", randomString);

    send(commSocket, randomString, strlen(randomString), 0);

    /**record time that challenge expires**/

    time_t currTime;
    time(&currTime);

    long challengeExpirationTime = currTime + CHALLENGE_TIMEOUT;

    debugPrintf("Challenge sent. Expires at: %ld\n", challengeExpirationTime);
    debugPrintf("Awaiting response . . .\n");

    /**Recv size of incoming username**/
    int totalBytes = 0;
    int BYTES_TO_RECEIVE = sizeof(int);
    int usernameLength;
    char usernameLengthBuffer[sizeof(int)];

    debugPrintf("Receiving username length . . .\n");

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &usernameLengthBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving username length");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    usernameLength = (*(int*)usernameLengthBuffer);

    debugPrintf("Username length: %d\n", usernameLength);

    debugPrintf("Receiving username . . .\n");

    /**Recv incoming username**/
    totalBytes = 0;
    BYTES_TO_RECEIVE = usernameLength;
    char username[BYTES_TO_RECEIVE + 1];

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &username[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving username");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    username[BYTES_TO_RECEIVE] = '\0';

    debugPrintf("Username: %s\n", username);

    debugPrintf("Receiving md5 hash . . .\n");

    /**Recv incoming hash**/
    totalBytes = 0;
    BYTES_TO_RECEIVE = sizeof(unsigned int);
    char hashBuffer[BYTES_TO_RECEIVE];
    unsigned int hash;

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &hashBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving md5 hash");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    hash = (unsigned int)(*(unsigned int *)hashBuffer);

    debugPrintf("Hashed received: %#x\n", hash);

    /**Make sure challenge isn't expired**/
    time(&currTime);
    if(currTime > challengeExpirationTime)
    {
        *user_out = NULL;
        return;
    }

    /**Get password (stored in memory) tied to username**/
    bank_user *user = NULL;

    for(int i = 0; i < NUM_USERS; i++)
    {
        if(strcasecmp(users[i].name, username) == 0)
        {
            user = &users[i];
            break;
        }
    }

    /**
     * Return failure if username not found.
     */
    if(user == NULL)
    {
        *user_out = NULL;
        return;
    }

    debugPrintf("User \"%s\" found\n", user->name);

    /**Compute md5 hash**/

    int preHashSize = strlen(user->name) + strlen(user->password) + CHALLENGE_SIZE + 1;
    char preHash[preHashSize];

    memcpy(preHash, user->name, strlen(user->name));
    memcpy(&preHash[strlen(user->name)], user->password, strlen(user->password));
    memcpy(&preHash[strlen(user->name) + strlen(user->password)], randomString, CHALLENGE_SIZE);
    preHash[preHashSize - 1] = '\0';

    debugPrintf("Pre-hashed string: %s\n", preHash);
    unsigned int result = *md5(preHash, strlen(preHash));
    debugPrintf("Hashed result: %#x\n", result);

    if(result != hash)
    {
        *user_out = NULL;
        return;
    }

    *user_out = user;
    return;
}

void handleRequest(int commSocket, bank_user *user)
{
    /**Receive request information**/
    int totalBytes = 0;
    int BYTES_TO_RECEIVE = sizeof(int) + sizeof(double);
    int requestType;
    double requestAmount;
    char requestBuffer[BYTES_TO_RECEIVE];

    debugPrintf("Receiving transaction request . . .\n");

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &requestBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving transaction request");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    requestType = *((int*)requestBuffer);
    requestAmount = *((double*)&requestBuffer[sizeof(int)]);
    debugPrintf("Received request. Type: %d -- Amount: %.2f\n", requestType, requestAmount);

    /**Handle request**/
    int response;
    double updatedBalance;

    if(requestType == TRANSACTION_DEPOSIT)
    {
        if(requestAmount > 0)
        {
            user->balance += requestAmount;
            response = RESPONSE_SUCCESS;
            updatedBalance = user->balance;

            printf("%s deposited $%.2f -- Balance: %.2f\n", user->name, requestAmount, user->balance);
            fflush(stdout);
        }
        else
        {
            response = RESPONSE_INVALID_REQUEST;
            updatedBalance = user->balance;

            printf("%s attempted to deposit a non-positive value. Denied.\n", user->name);
            fflush(stdout);
        }
    }
    else if(requestType == TRANSACTION_WITHDRAW)
    {
        if(requestAmount > 0)
        {
            if(user->balance >= requestAmount)
            {
                user->balance -= requestAmount;
                response = RESPONSE_SUCCESS;
                updatedBalance = user->balance;

                printf("%s withdrew $%.2f -- Balance: %.2f\n", user->name, requestAmount, user->balance);
                fflush(stdout);
            }
            else
            {
                response = RESPONSE_INSUFFICIENT_FUNDS;
                updatedBalance = user->balance;

                printf("%s attempted to withdraw %.2f, but has an insufficient balance of %.2f\n", user->name, requestAmount, user->balance);
                fflush(stdout);
            }
        }
        else
        {
            response = RESPONSE_INVALID_REQUEST;
            updatedBalance = user->balance;

            printf("%s attempted to withdraw a non-positive value. Denied.\n", user->name);
            fflush(stdout);
        }
    }
    else if(requestType == TRANSACTION_CHECKBAL)
    {
        response = RESPONSE_SUCCESS;
        updatedBalance = user->balance;

        printf("%s checked balance -- Balance: %.2f\n", user->name, user->balance);
        fflush(stdout);
    }
    else
    {
        //only will get here if server sends a non-valid request type
        response = RESPONSE_INVALID_REQUEST;
        updatedBalance = user->balance;
    }

    send(commSocket, &response, sizeof(response), 0);
    send(commSocket, &updatedBalance, sizeof(updatedBalance), 0);

    debugPrintf("Sent response: %d -- Updated Balance: %.2f\n", response, updatedBalance);
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