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
static size_t received, sent;
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

void on_int(int sig)
{
	time_t end, elapsed;

	time(&end);
	elapsed = end - start;

	printf("%zu %zu %zu %s\n", sent, received, elapsed, src_mac);
	exit(EXIT_SUCCESS);
}

void inline add_time(struct timeval *out, const struct timeval *start, int usecs)
{
	int carry;

	out->tv_usec = start->tv_usec + usecs;
	carry = out->tv_usec > 1000000;

	out->tv_sec = start->tv_sec + carry;
}

int inline time_interval(struct timeval *out, const struct timeval *start, const struct timeval *stop)
{
	out->tv_sec  = stop->tv_sec  - start->tv_sec;
	out->tv_usec = stop->tv_usec - start->tv_usec;

	if (out->tv_usec < 0 || out->tv_sec < 0) {
		out->tv_usec = out->tv_sec = 0;
		return -1;
	}

	return 0;
}

void loop() NORETURN;
void loop()
{
	char data[2*1024];
	char buffer[2*1024];
	size_t len = sizeof(data);
	ssize_t ret;
	fd_set rfds;

	sprintf(data, "MSG %s foo", src_mac);
	sleep_time = sleep_time < 0 ? 1 : sleep_time;

	send(sock, data, len, 0);
	sleep(1); /* make sure everyone's signed up */

	time(&start);

	while (1) {
		struct timeval tv_start, tv_end, tv;

		if (send(sock, data, len, MSG_DONTWAIT) > -1)
			sent++;

		if (gettimeofday(&tv_start, NULL) < 0) {
			perror("gettimeofday");
			exit(EXIT_FAILURE);
		}

		add_time(&tv_end, &tv_start, 10000);
		time_interval(&tv, &tv_start, &tv_end);

		while (1) {
			FD_ZERO(&rfds);
			FD_SET(sock, &rfds);
			int selret = select(sock + 1, &rfds, NULL, NULL, &tv);
			if (selret < 0)
				perror("select");

			if (selret == 0)
				break; /* timeout, time to send */

			if (selret) {
				ret = recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT);
				if (ret < 0)
					perror("recv");
				else
					received++;
			}

			gettimeofday(&tv_start, NULL);
			if (time_interval(&tv, &tv_start, &tv_end) < 0)
				break;
		}
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
	loop();
}
