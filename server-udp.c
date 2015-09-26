#include "banking.h"
#include "md5.h"

#define NUM_USERS 5
#define MAX_CONNECTIONS 10

void generateRandomString(char*, const int);

bank_user users[NUM_USERS];

client_state clients[MAX_CONNECTIONS]; //client states
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

        int incomingIp;
        int incomingPort;

        char outBuffer[68];

        socket_address clientAddress;
        int clientAddressLength = sizeof(clientAddress);

        debugPrintf("Waiting for message . . .\n");

        //receive incoming messages
        recvfrom(serverSocket, inBuffer, sizeof(inBuffer), 0, (struct sockaddr*)&clientAddress, &clientAddressLength);

        //record ip and port
        incomingIp = clientAddress.sin_addr.s_addr;
        incomingPort = clientAddress.sin_port;

        //extract message type
        inMsgType = *((int*)inBuffer);

        debugPrintf("Received from %s:%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);
        debugPrintf("Message type: %d\n", inMsgType);

        /*****PERFORM ACTION BASED ON MESSAGE TYPE*****/

        /**CHALLENGE REQUESTED**/
        if(inMsgType == REQ_CHALLENGE)
        {
            /**generate challenge**/
            char randomString[CHALLENGE_SIZE + 1];
            generateRandomString(randomString, CHALLENGE_SIZE);
            randomString[CHALLENGE_SIZE] = '\0';

            //calculate expiration time of challenge
            time_t currTime;
            time(&currTime);
            long challengeExpirationTime = currTime + CHALLENGE_TIMEOUT;
            
            debugPrintf("Challenge sent. Expires at: %ld\n", challengeExpirationTime);

            /**record ip address and record the challenge for it**/
            //first search for existing connections and override the challenge with the one we are sending out
            bool connectionAlreadyExists = false;
            for(int i = 0; i < MAX_CONNECTIONS; i++)
            {
                if(clients[i].ip == incomingIp && clients[i].port == incomingPort)
                {
                    connectionAlreadyExists = true;
                    memcpy(clients[i].challenge, randomString, CHALLENGE_SIZE);
                    clients[i].challengeExpiration = challengeExpirationTime;
                    break;
                }
            }

            //if no existing connection with ip:port, store information for this connection
            if(!connectionAlreadyExists)
            {
                clients[clientNumber].ip = incomingIp;
                clients[clientNumber].port = incomingPort;
                memcpy(clients[clientNumber].challenge, randomString, CHALLENGE_SIZE);
                clients[clientNumber].challengeExpiration = challengeExpirationTime;

                clientNumber++;
                clientNumber % MAX_CONNECTIONS;
                debugPrintf("Initializing state for connection: %s:%d\n", inet_ntoa(clientAddress.sin_addr), incomingPort);
            }

            *((int*)&outBuffer[0]) = CHALLENGE;
            memcpy(&outBuffer[sizeof(int)], randomString, CHALLENGE_SIZE);

            debugPrintf("Sending challenge: %s\n", randomString);

            sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
        }

        /**CHALLENGE ANSWER + TRANSACTION REQUEST**/
        else if(inMsgType == REQ_TRANSACTION)
        {
            /**ensure that there is an active connection with the ip/port. otherwise, no action**/
            client_state *connected_client = NULL;
            for(int i = 0; i < MAX_CONNECTIONS; i++)
            {
                if(clients[i].ip == incomingIp && clients[i].port == incomingPort)
                {
                    connected_client = &clients[i];
                    break;
                }
            }

            if(connected_client != NULL)
            {
                //unpack the in-buffer
                unsigned int hash;
                int trans_type;
                double trans_amount;
                unsigned int trans_id;
                char* username;

                hash = *((unsigned int *)&inBuffer[4]);
                trans_type = *((int *)&inBuffer[8]);
                trans_amount = *((double *)&inBuffer[12]);
                trans_id = *((unsigned int *)&inBuffer[20]);
                username = &inBuffer[24];

                debugPrintf("Received transaction request from: %s:%d\n", inet_ntoa(clientAddress.sin_addr), incomingPort);
                debugPrintf("Hash: %#x - Type: %d - Amount: %.2f - Id: %u - User: %s\n", hash, trans_type, trans_amount, trans_id, username);

                *((int*)outBuffer) = RESPONSE;
            
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
                 * Send auth_fail if user not found
                 */
                if(user == NULL)
                {
                    debugPrintf("User not found. Auth failed.\n");
                    *((int*)&outBuffer[4]) = AUTH_FAIL;
                    sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
                    continue;
                }

                /**Compute md5 hash**/
                int preHashSize = strlen(user->name) + strlen(user->password) + CHALLENGE_SIZE + 1;
                char preHash[preHashSize];

                memcpy(preHash, user->name, strlen(user->name));
                memcpy(&preHash[strlen(user->name)], user->password, strlen(user->password));
                memcpy(&preHash[strlen(user->name) + strlen(user->password)], connected_client->challenge, CHALLENGE_SIZE);
                preHash[preHashSize - 1] = '\0';

                debugPrintf("Pre-hashed string: %s\n", preHash);
                unsigned int result = *md5(preHash, strlen(preHash));
                debugPrintf("Hashed result: %#x\n", result);

                /**
                 * Send auth_fail if challenge is expired
                 */
                time_t currTime;
                time(&currTime);
                long curr = currTime;
                
                if(curr > connected_client->challengeExpiration)
                {
                    debugPrintf("Challenge Expired. Auth failed.\n");
                    *((int*)&outBuffer[4]) = AUTH_FAIL;
                    sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
                    continue;
                }

                /**
                 * Send auth_fail is hashes unequel
                 */
                if(result != hash)
                {
                    debugPrintf("Hashes unequal. Auth failed.\n");
                    *((int*)&outBuffer[4]) = AUTH_FAIL;
                    sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
                    continue;
                }

                /*****USER IS AUTHORIZED****/
                debugPrintf("User is authorized to make transaction.\n");

                //detect duplicate transactions
                if(trans_type != TRANSACTION_CHECKBAL && connected_client->last_trans_id == trans_id)
                {
                    debugPrintf("Duplicate transaction request detected. Re-sending response.\n");

                    //only last succesful transaction for an IP gets it's id stored, since failed transactions
                    //don't change account balance anyways
                    *((int*)&outBuffer[4]) = RESPONSE_SUCCESS;
                    *((double*)&outBuffer[8]) = user->balance;
                    sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
                    continue;
                }

                //handle non-duplicate transactions
                if(trans_type == TRANSACTION_DEPOSIT)
                {
                    if(trans_amount > 0)
                    {
                        user->balance += trans_amount;
                        *((int*)&outBuffer[4]) = RESPONSE_SUCCESS;
                        connected_client->last_trans_id = trans_id;

                        printf("%s deposited $%.2f -- Balance: %.2f\n", user->name, trans_amount, user->balance);
                        fflush(stdout);
                    }
                    else
                    {
                        *((int*)&outBuffer[4]) = RESPONSE_INVALID_REQUEST;

                        printf("%s attempted to deposit a non-positive value. Denied.\n", user->name);
                        fflush(stdout);
                    }
                }
                else if(trans_type == TRANSACTION_WITHDRAW)
                {
                    if(trans_amount > 0)
                    {
                        if(user->balance >= trans_amount)
                        {
                            user->balance -= trans_amount;
                            *((int*)&outBuffer[4]) = RESPONSE_SUCCESS;
                            connected_client->last_trans_id = trans_id;

                            printf("%s withdrew $%.2f -- Balance: %.2f\n", user->name, trans_amount, user->balance);
                            fflush(stdout);
                        }
                        else
                        {
                            *((int*)&outBuffer[4]) = RESPONSE_INSUFFICIENT_FUNDS;

                            printf("%s attempted to withdraw %.2f, but has an insufficient balance of %.2f\n", user->name, trans_amount, user->balance);
                            fflush(stdout);
                        }
                    }
                    else
                    {
                        *((int*)&outBuffer[4]) = RESPONSE_INVALID_REQUEST;

                        printf("%s attempted to withdraw a non-positive value. Denied.\n", user->name);
                        fflush(stdout);
                    }
                }
                else if(trans_type == TRANSACTION_CHECKBAL)
                {
                    *((int*)&outBuffer[4]) = RESPONSE_SUCCESS;

                    printf("%s checked balance -- Balance: %.2f\n", user->name, user->balance);
                    fflush(stdout);
                }
                else
                {
                    //only will get here if server sends a non-valid request type
                    *((int*)&outBuffer[4]) = RESPONSE_INVALID_REQUEST;
                }


                *((double*)&outBuffer[8]) = user->balance;
                debugPrintf("Sending response. Type: %d - Response: %d - Updated Balance %.2f\n", *((int*)&outBuffer[0]), *((int*)&outBuffer[4]), *((double*)&outBuffer[8]));
                sendto(serverSocket, outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
            }
            else
            {
                debugPrintf("Received transaction request from unchallenged ip/port. Doing nothing.\n");
            }
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