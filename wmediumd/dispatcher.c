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
#include "proto.h"

/* This is to make config.c happy */
double *prob_matrix = NULL;
int size = 0;
struct jammer_cfg jam_cfg;

extern int array_size;

/* Performance statistics */
static struct timeval tv;
static long int processed;
static long int processed_size;
static size_t recvd, sent;

struct peer {
	struct sockaddr_in addr;
	int active;
};

static struct peer *peers;
static int sock;

void on_int(int sig)
{
	fprintf(stderr, "dispatcher, recvd %zu, sent %zu\n", recvd, sent);
	exit(EXIT_SUCCESS);
}

int set_ip(const char *addr, struct sockaddr_in *sockaddr)
{
	struct mac_address mac = string_to_mac_address(addr);
	int pos = find_pos_by_mac_address(&mac);
	struct peer *peer = &peers[pos];
	memcpy(&peer->addr, sockaddr, sizeof(struct sockaddr_in));
	peer->active = 1;

	return pos;
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

	sent++;
	return 0;
}

inline void ovewrite_destination(unsigned char *ptr, struct mac_address *dest)
{
	char addr[17];
	size_t offset = strlen("MSG ");

	mac_address_to_string(addr, dest);
	memcpy(ptr + offset, addr, 16);
}

int relay_msg(unsigned char *ptr, size_t len, int pos)
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

		ovewrite_destination(ptr, to_mac);
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

	signal(SIGINT, on_int);
	srand(15); /* it's all pseudo-random anyway */
	while (1) {
		unsigned char buf[8*1024], *ptr;
		struct sockaddr_in from;
		struct wmd_msg msg;
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

		if (parse_msg(&msg, buf, len) < 0) {
			fprintf(stderr, "Bad message");
			continue;
		}

		printf("got message: %s\n", buf);

		/* Figure out what IP/port this was sent from and store it for the MAC */
		pos = set_ip(msg.addr, &from);

		if (msg.ack)
			printf("Got ACK: %s\n", buf);
		if (msg.ping)
			continue;

		recvd++;
		/* And send the message to the other nodes */
		relay_msg(buf, len, pos);
	}
}
