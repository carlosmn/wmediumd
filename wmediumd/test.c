#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include "mac_address.h"

int main(int argc, char **argv)
{
	void *context = zmq_init(1);
	void *zsock = zmq_socket(context, ZMQ_REQ);
	zmq_connect(zsock, "tcp://localhost:5555");
	zmq_msg_t req, reply;
	const char rnd[] = "pretend I'm random data";
	struct mac_address addr;

	while(1){
		/* Source */
		zmq_msg_init_size(&req, sizeof(struct mac_address));
		addr = string_to_mac_address("40:00:00:00:00:00");
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

		zmq_msg_init(&reply);
		zmq_recv(zsock, &reply, 0);

		printf ("Data sent, reply: %s\n", zmq_msg_data(&reply));
		zmq_msg_close(&reply);
	}
}
