#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>



/************UDP***************/
#define UDP_TIMEOUT             2

//udp message types
#define REQ_CHALLENGE           1
#define CHALLENGE               2
#define REQ_TRANSACTION         4
#define RESPONSE                8 

typedef struct
{
    int ip;
    int port;
    char challenge[64];
    long challengeExpiration;
    unsigned int last_trans_id;
} client_state;


/*******************************/

//transaction types
#define TRANSACTION_DEPOSIT     1
#define TRANSACTION_WITHDRAW    2
#define TRANSACTION_CHECKBAL    4

//auth responses
#define AUTH_SUCCESS    0
#define AUTH_FAIL       -4

//transaction responses
#define RESPONSE_SUCCESS            0
#define RESPONSE_INSUFFICIENT_FUNDS -1
#define RESPONSE_INVALID_REQUEST    -2


#define CHALLENGE_SIZE      64
#define CHALLENGE_TIMEOUT   10

typedef struct sockaddr_in socket_address;

bool debugMode = false;

typedef struct
{
    char *name;
    char *password;
    double balance;
} bank_user;

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
    fflush(stdout);
}

void initUsers(bank_user *users)
{
    users[0].name = "andrew";
    users[0].password = "bankpw";
    users[0].balance = 250.00;

    users[1].name = "drevil";
    users[1].password = "minime123";
    users[1].balance = 367.00;

    users[2].name = "williamgates";
    users[2].password = "microsoft";
    users[2].balance = 100000.00;

    users[3].name = "franz";
    users[3].password = "password";
    users[3].balance = 10.00;

    users[4].name = "jello";
    users[4].password = "pudding";
    users[4].balance = 115.00;
}