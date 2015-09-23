all: remotebank-tcp server-tcp

remotebank-tcp: remotebank-tcp.c
	c99 -o remotebank-tcp remotebank-tcp.c

server-tcp: server-tcp.c
	c99 -o server-tcp server-tcp.c