#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "proto.h"

const char *input[] = {
	"MSG 42 c4:85:08:10:d2:36 message content",
	"MSG 4422 c4:85:08:10:d2:37 message content",
	"ACK 42 c4:85:08:10:d2:37 c4:85:08:10:d2:36"
};

struct {
	unsigned int ack;
	char *src;
	unsigned long cookie;
	char *payload; /* message contents or source */
} output[] = {
	{0, "c4:85:08:10:d2:36", 42, "message content"},
	{0, "c4:85:08:10:d2:37", 4422, "message content"},
	{1, "c4:85:08:10:d2:37", 42, NULL},
};

int main(int argc, char **argv)
{
	int i;

	for(i = 0; i < 3; i++) {
		struct wmd_msg msg;

		assert(!parse_msg(&msg, input[i], strlen(input[i])));

		assert(msg.ack == output[i].ack);
		assert(msg.cookie == output[i].cookie);
		assert(!strcmp(msg.addr, output[i].src));

		if (!msg.ack) {
			assert(!strcmp(msg.data, output[i].payload));
		}

		printf(".");
	}

	for(i = 0; i < 3; i++) {
		char buffer[2*1024];
		struct wmd_msg msg;
		ssize_t sz;

		if (output[i].ack) {
			sz = fmt_ack(buffer, sizeof(buffer), output[i].cookie, output[i].src);
		} else {
			sz = fmt_msg(buffer, sizeof(buffer), output[i].cookie, output[i].src);
		}

		assert(!parse_msg(&msg, buffer, sz));
		assert(!strcmp(msg.addr, output[i].src));
		assert(msg.ack == output[i].ack);
		assert(msg.cookie == output[i].cookie);
		
		printf(".");
	}

	puts("\nAll OK");
	return 0;
}
