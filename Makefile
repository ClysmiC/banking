all: remotebank-tcp server-tcp remotebank-udp server-udp

remotebank-tcp: remotebank-tcp.c banking.h md5.h
	c99 -o remotebank-tcp remotebank-tcp.c

server-tcp: server-tcp.c banking.h md5.h
	c99 -o server-tcp server-tcp.c

remotebank-udp: remotebank-udp.c banking.h md5.h
	c99 -o remotebank-udp remotebank-udp.c

server-udp: server-udp.c banking.h md5.h
	c99 -o server-udp server-udp.c