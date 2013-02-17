#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mac_address.h"
#include "wmediumd.h"
#include "ctl.h"

/* This is to make config.c happy */
double *prob_matrix = NULL;
int size = 0;
struct jammer_cfg jam_cfg;

/* Performance statistics */
static struct timeval tv;
static long int processed;
static long int processed_size;

static void *s_ctl;

void exit_handler(int signal)
{
	struct timeval end;
	long int diff;
	zmq_msg_t msg;

	if (gettimeofday(&end, NULL)){
		perror("gettimeofday");
		exit(EXIT_FAILURE);
	}

	/* Figure out how long we ran */
	diff = end.tv_sec - tv.tv_sec;
	fprintf(stderr, "Ran for %d seconds\n", diff);
	fprintf(stderr, "Average %ld xfer/sec\n", processed/diff);
	fprintf(stderr, "Average %ld bytes/sec\n", processed_size/diff);

	zmq_msg_init_data(&msg, CTL_BYE, strlen(CTL_BYE), NULL, NULL);
	zmq_send(s_ctl, &msg, 0);
	zmq_close(&msg);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int sock;
	struct sockaddr_in addr;

	memset(&jam_cfg, 0, sizeof(jam_cfg));
	load_config("dispatcher-config.cfg");

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(5555);

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		return EXIT_FAILURE;
	}

	while (1) {
		unsigned char buf[2048];
		struct sockaddr_in from;
		struct msghdr msg;
		socklen_t fromlen;
		ssize_t len;

		fromlen = sizeof(from);
		len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &from, &fromlen);
		if (len < 0) {
			perror("recvmsg");
			return EXIT_FAILURE;
		}

		/*
		 * Figure out if this is a hello packet or a message packet. Each
		 * would get their own function. A hello packet adds the data we
		 * need such as IP to a structure. A message packet would go
		 * through the neighbours and send if necessary.
		 */

		if (!memcmp(buf, "HELLO", strlen("HELLO"))) {
			/* associate this IP with the MAC in the message */
		}

		if (!memcmp(buf, "MSG", strlen("MSG"))) {
			/* rand() and send the messages we need to */
		}
	}
}

int xmain(int argc, char **argv)
{
	void *ctx, *s, *s_pub;
	char mac[18] = {0};

	ctx = zmq_init(1);
	/* Receives messages from the interfaces */
	s = zmq_socket(ctx, ZMQ_PULL);
	zmq_bind(s, "tcp://*:5555");
	/* Sends messages to the interfaces */
	s_pub = zmq_socket(ctx, ZMQ_PUB);
	zmq_bind(s_pub, "tcp://*:5556");
	/* Sends control information to the interfaces */
	s_ctl = zmq_socket(ctx, ZMQ_PUSH);
	zmq_bind(s, "tcp://*:5557");

	memset(&jam_cfg, 0, sizeof(jam_cfg));
	load_config("dispatcher-config.cfg");

	if (gettimeofday(&tv, NULL)) {
		perror("gettimeofday");
		return EXIT_FAILURE;
	}

	if (signal(SIGINT, exit_handler) == SIG_ERR) {
		perror("signal");
		return EXIT_FAILURE;
	}

	while (1) {
		zmq_msg_t msg, dst, src;
		int64_t more;
		size_t more_size;

		zmq_msg_init(&src);
		zmq_recv(s, &src, 0);

		mac_address_to_string(mac, zmq_msg_data(&src));

		printf("Frame %s ", mac);

		more = 0;
		more_size = sizeof(more);
		zmq_getsockopt(s, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive three parts. Something went wrong */
			char *str = "ERROR: Destination is missing";
			fputs(str, stderr);
			continue;
		}

		/* Grab the scond part which is the destination */
		zmq_msg_init(&dst);
		zmq_recv(s, &dst, 0);
		mac_address_to_string(mac, zmq_msg_data(&dst));
		printf("-> %s\n", mac);

		more = 0;
		more_size = sizeof(more);
		zmq_getsockopt(s, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive three parts. Something went wrong */
			char *str = "ERROR: Message is missing";
			fputs(str, stderr);
			continue;
		}

		zmq_msg_init(&msg);
		zmq_recv(s, &msg, 0);
		processed_size += zmq_msg_size(&msg);

		zmq_send(s_pub, &dst, ZMQ_SNDMORE); /* For the SUB filter */
		zmq_send(s_pub, &src, ZMQ_SNDMORE);
		zmq_send(s_pub, &msg, 0);

		zmq_msg_close(&src);
		zmq_msg_close(&dst);
		zmq_msg_close(&msg);

		processed++;
	}

	zmq_close(s);
	zmq_term(ctx);

	return EXIT_SUCCESS;
}
