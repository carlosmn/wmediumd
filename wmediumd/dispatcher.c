#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mac_address.h"


static char hexen[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static void mac_address_to_string(char *str, const struct mac_address *mac)
{
	int i;
	for (i = 0; i < 6; i++) {
		str[i*2] = hexen[(mac->addr[i] & 0xf0) >> 4];
		str[i*2+1] = hexen[mac->addr[i] & 0x0f];
	}

}

int main(int argc, char **argv)
{
	void *ctx, *s;
	char mac[13] = {0};

	ctx = zmq_init(1);
	s = zmq_socket(ctx, ZMQ_REP);
	zmq_bind(s, "tcp://*:5555");

	while (1) {
		zmq_msg_t msg;
		size_t more, more_size;

		zmq_msg_init(&msg);
		zmq_recv(s, &msg, 0);

		mac_address_to_string(mac, zmq_msg_data(&msg));
		printf("Frame %s ", mac);
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
		mac_address_to_string(mac, zmq_msg_data(&msg));
		printf("-> %s\n", mac);

		more = 0;
		more_size = sizeof(more);
		zmq_getsockopt(s, ZMQ_RCVMORE, &more, &more_size);

		if (!more) {
			/* We should receive two parts. Something went wrong */
			char *str = "ERROR: Message is missing";
			puts(str);
			zmq_msg_init_data(&msg, str, strlen(str) + 1, NULL, NULL);
			zmq_send(s, &msg, 0);
			continue;
		}

		zmq_msg_init(&msg);
		zmq_recv(s, &msg, 0);

		/* msg now holds the actual frame the iface wanted to send */

		zmq_msg_init_data(&msg, "OK", strlen("OK") + 1, NULL, NULL);
		zmq_send(s, &msg, 0);

		zmq_msg_close(&msg);
	}

	/* TODO: catch C-x */
	zmq_close(s);
	zmq_term(ctx);

	return EXIT_SUCCESS;
}
