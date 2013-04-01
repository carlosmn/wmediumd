#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> /* getaddrinfo */

#include <unistd.h> /* getopt */
#include <signal.h>
#include <time.h>

#define NORETURN __attribute__((noreturn))

static char *src_mac;
static int sock, sleep_time = -1;
static size_t received;
static time_t start;

void die_help() NORETURN;
void die_help()
{
	fprintf(stderr, "usage: -s <src> [-t <delay>]\n");
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

	/* Hack to let many processes say hello and start hammering the dispatcher */
	sleep(1);
}

void on_int(int sig)
{
	time_t end, elapsed;
	int persec;

	time(&end);
	elapsed = end - start;

	persec = received / elapsed;

	printf("Received %zu bytes in %zu seconds, %zu bytes/sec for %s\n", received, elapsed, persec, src_mac);
	exit(EXIT_SUCCESS);
}

void loop() NORETURN;
void loop()
{
	char data[8*1024];
	char buffer[8*1024];
	size_t len = sizeof(data);
	ssize_t ret;

	/* so it has MSG, we send the whole 1k */
	memcpy(data, "MSG foo", strlen("MSG foo"));
	sleep_time = sleep_time < 0 ? 1 : sleep_time;

	time(&start);

	while (1) {
		send(sock, data, len, MSG_DONTWAIT);
		if ((ret = recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT)) > -1)
			received += ret;

		sleep(sleep_time);
	}
}


int main(int argc, char **argv) NORETURN;
int main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "s:t:")) != -1) {
		switch (c) {
		case 's':
			src_mac = strdup(optarg);
			break;
		case 't':
			sleep_time = atoi(optarg);
			break;
		default:
			die_help();
			break;
		}
	}

	if (!src_mac)
		die_help();

	signal(SIGINT, on_int);
	setup_sock();
	say_hello();
	loop();
}
