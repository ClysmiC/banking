#include "banking.h"
#include "md5.h"

#define NUM_USERS 5

void generateRandomString(char*, const int);
void authenticate(int, bank_user**);

bank_user users[NUM_USERS];

typedef struct
{
    char challenge[CHALLENGE_SIZE] ;
    long expirationTime;

} challenge_t;

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
        authenticate(commSocket, &authenticatedUser);


        if(authenticatedUser == NULL)
        {
            // printf("Failed authentication attempt.");
            close(commSocket);
            debugPrintf("Closing client socket.\n");
        }
        else
        {
        }
    }
}

void authenticate(int commSocket, bank_user **user_out)
{
    /**send challenge**/
    char randomString[CHALLENGE_SIZE + 1];
    generateRandomString(randomString, CHALLENGE_SIZE);
    randomString[CHALLENGE_SIZE] = '\0';

    debugPrintf("Sending challenge: %s\n", randomString);

    send(commSocket, randomString, sizeof(randomString), 0);

    /**record time that challenge expires**/
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

    challenge_t challenge;
    memcpy(challenge.challenge, randomString, CHALLENGE_SIZE);
    challenge.expirationTime = ms + CHALLENGE_TIMEOUT;

    debugPrintf("Challenge sent. Expires at: %ld\n", challenge.expirationTime);
    debugPrintf("Awaiting response . . .\n");

    /**Recv size of incoming username**/
    int totalBytes = 0;
    int BYTES_TO_RECEIVE = sizeof(int);
    int usernameLength;
    char usernameLengthBuffer[4];

    debugPrintf("Receiving username length . . .\n");

    while(totalBytes < BYTES_TO_RECEIVE)
    {
        int received;
        if((received = recv(commSocket, &usernameLengthBuffer[totalBytes], BYTES_TO_RECEIVE - totalBytes, 0)) < 0)
            dieWithError("Failed receiving username length");

        totalBytes += received;
        debugPrintf("Received %d bytes\n", received);
    }

    usernameLength = (int)(*(int*)usernameLengthBuffer);

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

    debugPrintf("Hashed result: %#x\n", hash);

    /**Get password (stored in memory) tied to username**/
    *user_out = NULL;

    for(int i = 0; i < NUM_USERS; i++)
    {
        if(strcasecmp(users[i].name, username) == 0)
        {
            *user_out = &users[i];
            break;
        }
    }

    /**
     * Return failure if username not found.
     */
    if(*user_out == NULL)
    {
        return;
    }

    debugPrintf("User \"%s\" found", (*user_out)->name);

    while(true){}

    //compare md5 hash

    //if same, output corresponding user

    //else output null
    user_out = NULL;
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