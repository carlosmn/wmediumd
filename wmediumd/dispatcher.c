#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mac_address.h"
#include "wmediumd.h"
#include "probability.h"
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

struct in_addr *ip_addr; /* IP address table */

int set_ip(char *ptr, ssize_t len, struct in_addr *in_addr)
{
	size_t minlen = strlen("HELLO XX:XX:XX:XX:XX:XX\n");
	size_t hellolen = strlen("HELLO ");

	if (len < minlen) {
		fprintf(stderr, "short packet");
		return -1;
	}

	ptr[minlen] = '\0'; /* bit of a hack for the next thing to work */
	struct mac_address mac = string_to_mac_address(ptr);

	int pos = find_pos_by_mac_address(&mac);
	struct in_addr *addr = &ip_addr[pos];
	addr->s_addr = in_addr->s_addr;

	return 0;
}

int main(int argc, char **argv)
{
	int sock;
	struct sockaddr_in addr;

	memset(&jam_cfg, 0, sizeof(jam_cfg));
	load_config("dispatcher-config.cfg");

	/* Allocate the IP table */
	ip_addr = malloc(sizeof(struct sockaddr) * size);
	if (!ip_addr) {
		perror("ip_addr");
		return EXIT_FAILURE;
	}

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
			set_ip(buf, len, &from.sin_addr);
		}

		if (!memcmp(buf, "MSG", strlen("MSG"))) {
			/* rand() and send the messages we need to */
		}
	}
}
