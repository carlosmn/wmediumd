#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> /* getaddrinfo */

#include <unistd.h> /* getopt */

#define NORETURN __attribute__((noreturn))

static char *dst_mac, *src_mac;
static int time, sock;

void die_help() NORETURN;
void die_help()
{
	fprintf(stderr, "usage: -s <src> -d <dst>\n");
	exit(EXIT_FAILURE);
}

void setup_sock()
{
	int ret;
	struct sockaddr_in sin;
	struct addrinfo *result, *res, hints;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if ((ret = getaddrinfo("localhost", NULL, &hints, &result)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	memcpy(&sin, result->ai_addr, result->ai_addrlen);
	sin.sin_port = htons(5555);
	if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);
}

int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "s:d:")) != -1) {
		switch (c) {
		case 's':
			dst_mac = strdup(optarg);
			break;
		case 'd':
			src_mac = strdup(optarg);
			break;
		case 't':
			time = atoi(optarg);
			break;
		default:
			die_help();
			break;
		}
	}

	if (!dst_mac && !src_mac)
		die_help();

	setup_sock();

	return 0;
}
