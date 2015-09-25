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
    char outBuffer[64];
    char *challenge = &inBuffer[4]; //points to challenge within inBuffer

    /**Send challenge request and listen for response**/
    bool ackReceived = false;

    //put message type into out buffer
    *((int*)outBuffer) = REQ_CHALLENGE;

    int attempts = 0;
    while(attempts < 10)
    {
        int bytesSent;

        if((bytesSent = sendto(commSocket, &outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < sizeof(outBuffer))
        {
            debugPrintf("Sent unexpected number of bytes. Expected: %d - Sent: %d\n", sizeof(outBuffer), bytesSent);
            dieWithError("Dying.");
        }

        debugPrintf("Sent challenge request\n");
        attempts++;

        /**Wait for response or timeout**/
        fd_set fds;
        struct timeval timeout;
        int rc, result;

        /* Set time limit. */
        timeout.tv_sec = UDP_TIMEOUT;
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

        debugPrintf("Challenge received: %s\n", challenge);
        ackReceived = true;
        break;
    }

    if(!ackReceived)
    {
        dieWithError("Did not receive challenge from server.");
    }

    /**Fill out answer + request**/
    *((int*)outBuffer) = REQ_TRANSACTION;
    *((int*)&outBuffer[4]) = REQ_TRANSACTION;

    
    /**Hash user + pass + challenge**/
    int preHashSize = strlen(argv[2]) + strlen(argv[3]) + CHALLENGE_SIZE + 1;
    char preHash[preHashSize];
    memcpy(preHash, argv[2], strlen(argv[2]));
    memcpy(&preHash[strlen(argv[2])], argv[3], strlen(argv[3]));
    memcpy(&preHash[strlen(argv[2]) + strlen(argv[3])], challenge, CHALLENGE_SIZE);
    preHash[preHashSize - 1] = '\0';

    debugPrintf("Pre-hashed string: %s\n", preHash);
    unsigned int result = *md5(preHash, strlen(preHash));
    debugPrintf("Hashed result: %#x\n", result);

    *((int*)&outBuffer[4]) = result;

    /**Fill out transaction info**/
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

    *((int*)&outBuffer[8]) = requestType;
    *((double*)&outBuffer[12]) = requestAmount;

    debugPrintf("Transaction type: %d - Amount: %.2f\n", requestType, requestAmount);

    time_t t;
    srand((unsigned) time(&t));
    unsigned int id = (unsigned int)rand();
    *((unsigned int*)&outBuffer[20]) = id;

    debugPrintf("Transaction id: %u\n", id);

    /**Place user name at end of buffer**/
    memcpy(&outBuffer[24], argv[2], strlen(argv[2]));



    /**Send the answer + request**/
    attempts = 0;
    ackReceived = false;
    while(attempts < 10)
    {
        int bytesSent;

        if((bytesSent = sendto(commSocket, &outBuffer, sizeof(outBuffer), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < sizeof(outBuffer))
        {
            debugPrintf("Sent unexpected number of bytes. Expected: %d - Sent: %d\n", sizeof(outBuffer), bytesSent);
            dieWithError("Dying.");
        }

        debugPrintf("Sent answer + transaction request\n");
        attempts++;

        /**Wait for response or timeout**/
        fd_set fds;
        struct timeval timeout;
        int rc, result;

        /* Set time limit. */
        timeout.tv_sec = UDP_TIMEOUT;
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
            dieWithError("Received wrong number of bytes when receiving transaction response.");
        }

        int responseType = *((int*)inBuffer);

        debugPrintf("Received message type: %d\n", responseType);

        //make sure it is type response
        if(responseType != RESPONSE)
        {
            printf("Expected: %d - Actual: %d", RESPONSE, responseType);
            dieWithError("Received wrong message type");
        }

        int response;
        double updatedBalance;

        response = *((int*)&inBuffer[4]);
        updatedBalance = *((double*)&inBuffer[8]);

        /**PRINT RESULTS**/
        debugPrintf("Response: %d\n", response);

        if(response == AUTH_FAIL)
        {
            printf("\nAuthentication failed.\n");
            close(commSocket);
            exit(0);
        }

        debugPrintf("Updated Balance: %.2f\n", updatedBalance);

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

        ackReceived = true;
        break;
    }

    if(!ackReceived)
    {
        dieWithError("Did not receive response from server. Transaction may have succesfully occured. Please run checkbal when connection is regained.");
    }
}