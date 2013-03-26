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

static void *s_ctl;

struct sockaddr_in *ip_addr; /* IP address table */
static int sock;


int set_ip(const char *ptr, ssize_t len, struct sockaddr_in *sockaddr)
{
	char buf[64];
	size_t minlen = strlen("HELLO XX:XX:XX:XX:XX:XX\n");
	char *sp;

	if (len < minlen) {
		fprintf(stderr, "short packet");
		return -1;
	}

	size_t hellolen = strlen("HELLO ");
	size_t maclen = strlen("XX:XX:XX:XX:XX:XX");
	memcpy(buf, ptr + hellolen, maclen);
	buf[maclen] = '\0';

	struct mac_address mac = string_to_mac_address(ptr);
	int pos = find_pos_by_mac_address(&mac);
	struct sockaddr_in *addr = &ip_addr[pos];
	memcpy(addr, sockaddr, sizeof(struct sockaddr_in));

	sp = memchr(ptr + hellolen, '\n', len);
	if (!sp) {
		fprintf(stderr, "no LF after MAC");
		return -1;
	}

	return 0;
}

int find_pos_by_ip(struct in_addr *in_addr) {

	int i=0;

	void * ptr = ip_addr;
	while(memcmp(ptr, in_addr, sizeof(struct in_addr)) && i < array_size)
	{
		i++;
		ptr = ptr + sizeof(struct in_addr);
	}

	return ((i >= array_size) ?  -1 :  i);
}

int inline send_msg(const struct in_addr *in_addr, const unsigned char *msg, ssize_t len)
{
	ssize_t ret;

	if ((ret = sendto(sock, msg, len, 0, (struct sockaddr *) in_addr, sizeof(struct in_addr))) < 0) {
		perror("sendto");
		return -1;
	}

	return 0;
}

int relay_msg(const unsigned char *ptr, ssize_t len, struct sockaddr_in *sockaddr)
{
	int pos, i;
	double loss, closs;
	const unsigned char *msg;
	struct mac_address *from_mac;

	/* figure out who sent us this packet */
	if ((pos = find_pos_by_ip(&sockaddr->sin_addr)) < 0)
		return -1;

	/* and what its MAC address is */
	if ((from_mac = get_mac_address(pos)) == NULL) {
		fprintf(stderr, "failed to get MAC address for pos %d\n", pos);
		return -1;
	}

	msg = ptr + strlen("MSG ");
	len -= strlen("MSG ");
	loss = drand48();
	/* for each pair of addresses, find the loss at rate 0 and
	 * compare against the random value */

	for (i = 0; i < array_size; i++) {
		struct mac_address *to_mac = get_mac_address(i);
		closs = find_prob_by_addrs_and_rate(prob_matrix, from_mac, to_mac, 0);

		/* better luck next time */
		if (closs > loss)
			continue;

		send_msg(&ip_addr[i].sin_addr, msg, len);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;

	memset(&jam_cfg, 0, sizeof(jam_cfg));
	load_config("dispatcher-config.cfg");

	/* Allocate the IP table */
	ip_addr = malloc(sizeof(struct sockaddr_in) * size);
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

	srand(15); /* it's all pseudo-random anyway */
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
			set_ip(buf, len, &from);
		}

		if (!memcmp(buf, "MSG", strlen("MSG")) && len > strlen("MSG ")) {
			/* rand() and send the messages we need to */
			relay_msg(buf, len, &from);
		}
	}
}
