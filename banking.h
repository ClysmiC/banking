#define TRANSACTION_DEPOSIT 1
#define TRANSACTION_WITHDRAWAL 2

#define RESPONSE_SUCCESS 0
#define RESPONSE_INSUFFICIENT_FUNDS 1

typedef struct 
{
	int type;
	float amount;
} transaction_request;

typedef struct
{
	int response;
} transaction_response;