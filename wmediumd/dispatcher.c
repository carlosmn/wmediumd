#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "mac_address.h"
#include "wmediumd.h"

/* This is to make config.c happy */
double *prob_matrix = NULL;
int size = 0;
struct jammer_cfg jam_cfg;

/* Performance statistics */
static struct timeval tv;
static long int processed;
static long int processed_size;

void exit_handler(int signal)
{
	struct timeval end;
	long int diff;

	if (gettimeofday(&end, NULL)){
		perror("gettimeofday");
		exit(EXIT_FAILURE);
	}

	/* Figure out how long we ran */
	diff = end.tv_sec - tv.tv_sec;
	fprintf(stderr, "Ran for %d seconds\n", diff);
	fprintf(stderr, "Average %ld xfer/sec\n", processed/diff);
	fprintf(stderr, "Average %ld bytes/sec\n", processed_size/diff);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	void *ctx, *s, *s_pub;
	char mac[18] = {0};

	ctx = zmq_init(1);
	s = zmq_socket(ctx, ZMQ_PULL);
	zmq_bind(s, "tcp://*:5555");
	s_pub = zmq_socket(ctx, ZMQ_PUB);
	zmq_bind(s, "tcp://*:5556");

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
		size_t more, more_size;

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
