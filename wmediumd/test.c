#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> /* getaddrinfo */

#include <unistd.h> /* getopt */

#define NORETURN __attribute__((noreturn))

static char *dst_mac, *src_mac;
static int sock, time = -1;

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

void say_hello()
{
	char buf[64];
	ssize_t sent, len;

	sprintf(buf, "HELLO %s\n", src_mac);
	if (send(sock, buf, strlen(buf), 0) < 0) {
		perror("hello");
		exit(EXIT_FAILURE);
	}
}

void loop() NORETURN;
void loop()
{
	const char *data = "MSG foo!";
	const char buffer[1024];
	size_t len = strlen(data);

	time = time < 0 ? 1 : time;

	while (1) {
		send(sock, data, len, 0);
		if (recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT) > -1) {
			puts("I gots me a message");
		}
		sleep(time);
	}
}


int main(int argc, char **argv) NORETURN;
int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "s:d:t:")) != -1) {
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
	say_hello();
	loop();
}
