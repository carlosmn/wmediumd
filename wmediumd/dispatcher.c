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

extern int array_size;

/* Performance statistics */
static struct timeval tv;
static long int processed;
static long int processed_size;

struct peer {
	struct sockaddr_in addr;
	int active;
};

static struct peer *peers;
static int sock;


unsigned char *set_ip(int *pos_out, const unsigned char *ptr, ssize_t len, struct sockaddr_in *sockaddr)
{
	char buf[64];
	size_t minlen = strlen("XX:XX:XX:XX:XX:XX\n");
	unsigned char *sp;

	if (len < minlen) {
		fprintf(stderr, "short packet, %zd insead of >%zu\n", len, minlen);
		return NULL;
	}

	size_t maclen = strlen("XX:XX:XX:XX:XX:XX");
	memcpy(buf, ptr, maclen);
	buf[maclen] = '\0';

	struct mac_address mac = string_to_mac_address(buf);
	int pos = find_pos_by_mac_address(&mac);
	struct peer *peer = &peers[pos];
	peer->active = 1;
	memcpy(&peer->addr, sockaddr, sizeof(struct sockaddr_in));

	sp = memchr(ptr + maclen, '\n', len);
	if (!sp) {
		fprintf(stderr, "no LF after MAC\n");
		return NULL;
	}

	//printf("registered %s at %d\n", buf, pos);
	*pos_out = pos;
	return sp + 1;
}

int find_pos_by_addr(struct sockaddr_in *in_addr)
{
	int i;

	for (i = 0; i < array_size; i++) {
		if (!memcmp(in_addr, &peers[i].addr, sizeof(struct in_addr)))
			break;
	}

	return ((i >= array_size) ?  -1 :  i);
}

int inline send_msg(const struct sockaddr_in *sin, const unsigned char *msg, size_t len)
{
	ssize_t ret;

	if ((ret = sendto(sock, msg, len, 0, (struct sockaddr *) sin, sizeof(struct sockaddr_in))) < 0) {
		perror("sendto");
		return -1;
	}

	return 0;
}

int relay_msg(const unsigned char *ptr, size_t len, int pos)
{
	int i;
	double loss, closs;
	struct mac_address *from_mac;

	/* and what its MAC address is */
	if ((from_mac = get_mac_address(pos)) == NULL) {
		fprintf(stderr, "failed to get MAC address for pos %d\n", pos);
		return -1;
	}

	loss = drand48();
	/* for each pair of addresses, find the loss at rate 0 and
	 * compare against the random value */

	for (i = 0; i < array_size; i++) {
		if (i == pos)
			continue;

		struct mac_address *to_mac = get_mac_address(i);
		closs = find_prob_by_addrs_and_rate(prob_matrix, from_mac, to_mac, 0);

		//printf("peers[i].active %d, pos %d, i %d\n", peers[i].active, pos, i);
		/* better luck next time */
		if (!peers[i].active || closs > loss)
			continue;

		send_msg(&peers[i].addr, ptr, len);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;

	memset(&jam_cfg, 0, sizeof(jam_cfg));
	load_config("dispatcher-config.cfg");

	/* Allocate the peer table */
	peers = calloc(size, sizeof(struct peer));
	if (!peers) {
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

	srand(15); /* it's all pseudo-random anyway */
	while (1) {
		size_t msg_pfx_len = strlen("MSG ");
		unsigned char buf[8*1024], *ptr;
		struct sockaddr_in from;
		struct msghdr msg;
		socklen_t fromlen;
		ssize_t len;
		int pos;


		fromlen = sizeof(from);
		memset(&from, 0, sizeof(from));
		len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &from, &fromlen);
		if (len < 0) {
			perror("recvmsg");
			return EXIT_FAILURE;
		}

		if (memcmp(buf, "MSG ", msg_pfx_len)) {
			fprintf(stderr, "Received invalid message");
			continue;
		}

		/* Figure out what IP/port this was sent from and store it for the MAC */
		ptr = set_ip(&pos, buf + msg_pfx_len, len - msg_pfx_len, &from);
		if (!ptr)
			continue;

		/* And send the message to the  */
		relay_msg(ptr, len - (ptr - buf), pos);
	}
}
