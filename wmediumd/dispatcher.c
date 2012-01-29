#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	void *ctx, *s;

	ctx = zmq_init(1);
	s = zmq_socket(ctx, ZMQ_REP);
	zmq_bind(s, "tcp://*:5555");

	while (1) {
		zmq_msg_t msg;
		size_t more, more_size;

		zmq_msg_init(&msg);
		zmq_recv(s, &msg, 0);

		printf("Frame %s ", zmq_msg_data(&msg));
		zmq_msg_close(&msg);

		more = 0;
		more_size = sizeof(more);
		zmq_getsockopt(s, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive two parts. Something went wrong */
			char *str = "ERROR: Second part is missing";
			puts(str);
			zmq_msg_init_data(&msg, str, strlen(str) + 1, NULL, NULL);
			zmq_send(s, &msg, 0);
			continue;
		}

		/* Grab the scond part which is the destination */
		zmq_msg_init(&msg);
		zmq_recv(s, &msg, 0);
		printf("-> %s\n", zmq_msg_data(&msg));

		zmq_msg_init_data(&msg, "OK", strlen("OK") + 1, NULL, NULL);
		zmq_send(s, &msg, 0);

		zmq_msg_close(&msg);
	}

	/* TODO: catch C-x */
	zmq_close(s);
	zmq_term(ctx);

	return EXIT_SUCCESS;
}
