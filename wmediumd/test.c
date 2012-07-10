#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include "mac_address.h"
#include "ctl.h"

static const char *my_addr = "40:00:00:00:00:00";
static const char *other_addr = "40:00:00:00:10:00";
static const char *src_addr;
static const char *dst_addr;

static int swan(void *sock)
{
	zmq_msg_t msg;

	puts("in swan");
	zmq_msg_init(&msg);
	zmq_recv(sock, &msg, 0);
	if (memcpy(CTL_BYE, zmq_msg_data(&msg), strlen(CTL_BYE))) {
		puts("BAI");
		exit(EXIT_SUCCESS);
	}
	zmq_close(&msg);

	fputs("Unknown control code", stderr);
}

int main(int argc, char **argv)
{
	void *context = zmq_init(1);
	void *zsock = zmq_socket(context, ZMQ_PUSH);
	void *zsock_sub = zmq_socket(context, ZMQ_SUB);
	/* Receive instructions from the dispatcher */
	void *zsock_ctl = zmq_socket(context, ZMQ_PULL);
	zmq_msg_t req, reply, src;
	const char rnd[] = "pretend I'm random data";
	struct mac_address src_mac, dst_mac;
	int c, polls;
	zmq_pollitem_t polli[2];

	while ((c = getopt(argc, argv, "s:d:")) != -1) {
		switch (c) {
		case 's':
			src_addr = strdup(optarg);
			break;
		case 'd':
			dst_addr = strdup(optarg);
			break;
		case '?':
			fprintf(stderr, "Error parsing variables\n");
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	memset(polli, 0, sizeof(polli));
	polli[0].socket = zsock_sub;
	polli[0].events = ZMQ_POLLIN;
	polli[1].socket = zsock_ctl;
	polli[1].events = ZMQ_POLLIN;

	if (!src_addr)
		src_addr = my_addr;
	if (!dst_addr)
		dst_addr = other_addr;

	src_mac = string_to_mac_address(src_addr);
	dst_mac = string_to_mac_address(dst_addr);
	zmq_setsockopt(zsock_sub, ZMQ_SUBSCRIBE, &src_mac, sizeof(struct mac_address));
	zmq_connect(zsock, "tcp://localhost:5555");
	zmq_connect(zsock_sub, "tcp://localhost:5556");
	zmq_connect(zsock_ctl, "tcp://localhost:5557");

	while(1){
		zmq_msg_t msg, dst, src, s_msg, s_dst, s_src;
		int64_t more;
		size_t more_size = sizeof(more);

		/* Source */
		zmq_msg_init_data(&s_src, &src_mac, sizeof(struct mac_address), NULL, NULL);
		zmq_send(zsock, &s_src, ZMQ_SNDMORE);
		zmq_msg_close(&s_src);

		/* Destination */
		zmq_msg_init_data(&s_dst, &dst_mac, sizeof(struct mac_address), NULL, NULL);
		zmq_send(zsock, &s_dst, ZMQ_SNDMORE);
		zmq_msg_close(&s_dst);

		/* Data */
		zmq_msg_init_size(&req, sizeof(rnd));
		memcpy(zmq_msg_data(&req), rnd, sizeof(rnd));
		zmq_send(zsock, &req, 0);
		zmq_msg_close(&req);

		if ((polls = zmq_poll(polli, 2, 100000)) < 0) {
			perror("zmq_poll");
			exit(EXIT_FAILURE);
		}

		if (polls == 0) {
			puts("didn't get anything");
			continue;
		}

		if (polli[1].revents & ZMQ_POLLIN)
			swan(zsock_ctl);

		zmq_msg_init(&dst);

		more = 0;
		zmq_recv(zsock_sub, &dst, 0);
		zmq_getsockopt(zsock_sub, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive three parts. Something went wrong */
			char *str = "ERROR: Second part is missing";
			fputs(str, stderr);
			continue;
		}

		zmq_msg_init(&src);
		zmq_recv(zsock_sub, &src, 0);
		zmq_msg_close(&src);
		
		more = 0;
		zmq_getsockopt(zsock_sub, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive three parts. Something went wrong */
			char *str = "ERROR: Third part is missing";
			fputs(str, stderr);
			continue;
		}

		zmq_msg_init(&reply);
		//puts("recv more");
		zmq_recv(zsock_sub, &reply, 0);

		zmq_msg_close(&dst);
		zmq_msg_close(&reply);
		sleep(1);
	}
}
