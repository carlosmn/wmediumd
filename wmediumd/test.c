#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int main(int argc, char **argv)
{
	puts("Just started");

	void *context = zmq_init(1);
	void *zsock = zmq_socket(context, ZMQ_REQ);
	zmq_connect(zsock, "tcp://10.0.2.2:5555");
	zmq_msg_t req, reply;

	while(1){
		zmq_msg_init_size(&req, 13);
		memcpy(zmq_msg_data(&req), "400000000000", 13);
		zmq_send(zsock, &req, 0);
		zmq_msg_close(&req);

		zmq_msg_init(&reply);
		zmq_recv(zsock, &reply, 0);

		printf ("Data sent, reply: %s\n", zmq_msg_data(&reply));
		zmq_msg_close(&reply);
	}
}
