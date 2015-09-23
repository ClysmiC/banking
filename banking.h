#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>

#define TRANSACTION_DEPOSIT 1
#define TRANSACTION_WITHDRAWAL 2

#define AUTH_SUCCESS 0
#define AUTH_FAIL 1

#define RESPONSE_SUCCESS 0
#define RESPONSE_INSUFFICIENT_FUNDS 1

#define CHALLENGE_SIZE 64

typedef struct sockaddr_in socket_address;

bool debugMode = false;

typedef struct
{
	char *name;
	char *password;
	double balance;
} bank_user;

typedef struct 
{
	int type;
	float amount;
} transaction_request;

typedef struct
{
	int response;
} transaction_response;

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
	users[0].name = "Andrew";
	users[0].password = "bankpw";
	users[0].balance = 250.00;

	users[1].name = "DrEvil";
	users[1].password = "minime123";
	users[1].balance = 367.00;

	users[2].name = "WilliamGates";
	users[2].password = "microsoft";
	users[2].balance = 100000.00;

	users[3].name = "franz";
	users[3].password = "password";
	users[3].balance = 10.00;

	users[4].name = "jello";
	users[4].password = "pudding";
	users[4].balance = 115.00;
}