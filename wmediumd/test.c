#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include "mac_address.h"

static const char *my_addr = "40:00:00:00:00:00";

int main(int argc, char **argv)
{
	void *context = zmq_init(1);
	void *zsock = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(zsock, "tcp://localhost:5555");
	void *zsock_sub = zmq_socket(context, ZMQ_SUB);
	zmq_connect(zsock_sub, "tcp://localhost:5556");
	zmq_setsockopt(zsock_sub, ZMQ_SUBSCRIBE, my_addr, strlen(my_addr));
	zmq_msg_t req, reply, src;
	const char rnd[] = "pretend I'm random data";
	struct mac_address addr;
	zmq_pollitem_t polli[] = {
		{ zsock, 0, ZMQ_POLLIN, 0 },
		{ zsock_sub, 0, ZMQ_POLLIN, 0 },
	};

	while(1){
		zmq_msg_t msg, dst, src;
		size_t more, more_size;

		/* Source */
		zmq_msg_init_size(&req, sizeof(struct mac_address));
		addr = string_to_mac_address(my_addr);
		memcpy(zmq_msg_data(&req), &addr, sizeof(struct mac_address) );
		zmq_send(zsock, &req, ZMQ_SNDMORE);

		/* Destination */
		zmq_msg_init_size(&req, sizeof(struct mac_address));
		addr = string_to_mac_address("40:00:00:00:10:00");
		memcpy(zmq_msg_data(&req), &addr, sizeof(struct mac_address) );
		zmq_send(zsock, &req, ZMQ_SNDMORE);

		/* Data */
		zmq_msg_init_size(&req, sizeof(rnd));
		memcpy(zmq_msg_data(&req), rnd, sizeof(rnd));
		zmq_send(zsock, &req, 0);
		zmq_msg_close(&req);

		zmq_msg_init(&dst);
		zmq_recv(zsock_sub, &dst, 0);

		more = 0;
		more_size = sizeof(more);
		zmq_getsockopt(zsock_sub, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive two parts. Something went wrong */
			char *str = "ERROR: Second part is missing";
			fputs(str, stderr);
			continue;
		}

		zmq_msg_init(&reply);
		zmq_recv(zsock_sub, &reply, 0);

		zmq_msg_close(&dst);
		zmq_msg_close(&reply);
	}
}
