all: remotebank-tcp server-tcp

remotebank-tcp: remotebank-tcp.c banking.h md5.h
	c99 -o remotebank-tcp remotebank-tcp.c

server-tcp: server-tcp.c banking.h md5.h
	c99 -o server-tcp server-tcp.c