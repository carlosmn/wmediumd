/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author:	Javier Lopez 	<jlopex@cozybit.com>
 *		Javier Cardona	<javier@cozybit.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *	02110-1301, USA.
 */

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>

#include "wmediumd.h"
#include "probability.h"
#include "mac_address.h"
#include "ieee80211.h"
#include "config.h"
#include "proto.h"

struct nl_sock *sock;
struct nl_msg *msg;
struct nl_cb *cb;
struct nl_cache *cache;
struct genl_family *family;

int running = 0;
struct jammer_cfg jam_cfg;
double *prob_matrix;
int size;

static int nl_fd;  /* netlink fd */
static int disp_fd;   /* dispatcher fd */

static int received = 0;
static int sent = 0;
static int dropped = 0;
static int acked = 0;

static struct mac_address mac_addr;
static char *mac;
static char *dispatcher;
static int port = 5555;
static int background;


/*
 * 	Generates a random double value
 */

double generate_random_double()
{

	return rand()/((double)RAND_MAX+1);
}

/*
 *	Send a tx_info frame to the kernel space.
 */

int send_tx_info_frame_nl(struct mac_address *src,
			  unsigned int flags, int signal,
			  struct hwsim_tx_rate *tx_attempts,
			  unsigned long cookie)
{

	msg = nlmsg_alloc();
	if (!msg) {
		printf("Error allocating new message MSG!\n");
		goto out;
	}

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, genl_family_get_id(family),
		    0, NLM_F_REQUEST, HWSIM_CMD_TX_INFO_FRAME, VERSION_NR);

	int rc;
	rc = nla_put(msg, HWSIM_ATTR_ADDR_TRANSMITTER,
		     sizeof(struct mac_address), src);
	rc = nla_put_u32(msg, HWSIM_ATTR_FLAGS, flags);
	rc = nla_put_u32(msg, HWSIM_ATTR_SIGNAL, signal);
	rc = nla_put(msg, HWSIM_ATTR_TX_INFO,
		     IEEE80211_MAX_RATES_PER_TX *
		     sizeof(struct hwsim_tx_rate), tx_attempts);

	rc = nla_put_u64(msg, HWSIM_ATTR_COOKIE, cookie);

	if(rc!=0) {
		printf("Error filling payload\n");
		goto out;
	}

	nl_send_auto_complete(sock,msg);
	nlmsg_free(msg);
	return 0;
out:
	nlmsg_free(msg);
	return -1;
}

/*
 * 	Send a cloned frame to the kernel space.
 */

int send_cloned_frame_msg(struct mac_address *dst,
			  const char *data, int data_len, int rate_idx, int signal)
{

	msg = nlmsg_alloc();
	if (!msg) {
		printf("Error allocating new message MSG!\n");
		goto out;
	}

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, genl_family_get_id(family),
		    0, NLM_F_REQUEST, HWSIM_CMD_FRAME, VERSION_NR);

	int rc;
	rc = nla_put(msg, HWSIM_ATTR_ADDR_RECEIVER,
		     sizeof(struct mac_address), dst);
	rc = nla_put(msg, HWSIM_ATTR_FRAME, data_len, data);
	rc = nla_put_u32(msg, HWSIM_ATTR_RX_RATE, rate_idx);
	rc = nla_put_u32(msg, HWSIM_ATTR_SIGNAL, signal);

	if(rc!=0) {
		printf("Error filling payload\n");
		goto out;
	}

	nl_send_auto_complete(sock,msg);
	nlmsg_free(msg);
	return 0;
out:
	nlmsg_free(msg);
	return -1;
}

/*
 * 	Get a signal value by rate index
 */

int get_signal_by_rate(int rate_idx)
{
	const int rate2signal [] =
		{ -80,-77,-74,-71,-69,-66,-64,-62,-59,-56,-53,-50 };
	if (rate_idx >= 0 || rate_idx < IEEE80211_AVAILABLE_RATES)
		return rate2signal[rate_idx];
	return 0;
}

/*
 * 	Send a frame applying the loss probability of the link
 */

int send_frame_msg_apply_prob_and_rate(struct mac_address *src,
				       struct mac_address *dst,
				       char *data, int data_len, int rate_idx)
{

	/* At higher rates higher loss probability*/
	double prob_per_link = find_prob_by_addrs_and_rate(prob_matrix,
							   src,dst, rate_idx);
	double random_double = generate_random_double();

	if (random_double < prob_per_link) {
		printf("dropped\n");
		dropped++;
		return 0;
	} else {

		printf("sent\n");
		/*received signal level*/
		int signal = get_signal_by_rate(rate_idx);

		send_cloned_frame_msg(dst,data,data_len,rate_idx,signal);
		sent++;
		return 1;
	}
}

/*
 * 	Set a tx_rate struct to not valid values
 */

void set_all_rates_invalid(struct hwsim_tx_rate* tx_rate)
{
	int i;
	/* set up all unused rates to be -1 */
	for (i=0; i < IEEE80211_MAX_RATES_PER_TX; i++) {
        	tx_rate[i].idx = -1;
		tx_rate[i].count = 0;
	}
}

/*
 *	Determine whether we should be jamming this transmitting mac.
 */
int jam_mac(struct jammer_cfg *jcfg, struct mac_address *src)
{
	int jam = 0, i;

	for (i = 0; i < jcfg->nmacs; i++) {
		if (!memcmp(&jcfg->macs[i], src, sizeof(struct mac_address))) {
			jam = 1;
			break;
		}
	}
	return jam;
}

/*
 * 	Iterate all the radios and send a copy of the frame to each interface.
 */

void send_frames_to_radios_with_retries(struct mac_address *src, char*data,
					int data_len, unsigned int flags,
					struct hwsim_tx_rate *tx_rates,
					unsigned long cookie)
{

	struct mac_address *dst;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)data;
	struct hwsim_tx_rate tx_attempts[IEEE80211_MAX_RATES_PER_TX];

	int round = 0, tx_ok = 0, counter, i;

	if (jam_cfg.jam_all || jam_mac(&jam_cfg, src)) {
		printf("medium busy!\n");
		return;
	}
	/* We prepare the tx_attempts struct */
	set_all_rates_invalid(tx_attempts);

	while (round < IEEE80211_MAX_RATES_PER_TX &&
	       tx_rates[round].idx != -1 && tx_ok!=1) {

		counter = 1;

		/* Set rate index and flags used for this round */
		tx_attempts[round].idx = tx_rates[round].idx;

		while(counter <= tx_rates[round].count && tx_ok !=1 ) {

			/* Broadcast the frame to all the radio ifaces*/
			for (i=0;i<size;i++) {

				dst =  get_mac_address(i);

				/*
				 * If origin and destination are the
				 * same just skip this iteration
				*/
				if(memcmp(src,dst,sizeof(struct mac_address))
					  == 0 ){
					continue;
				}

				/* Try to send it to a radio and if
				 * the frame is destined to this radio tx_ok
				*/
				if(send_frame_msg_apply_prob_and_rate(
					src, dst, data, data_len,
					tx_attempts[round].idx) &&
					memcmp(dst, hdr->addr1,
					sizeof(struct mac_address))==0) {
						tx_ok = 1;
				}
			}
			tx_attempts[round].count = counter;
			counter++;
		}
		round++;
	}

	if (tx_ok) {
		/* if tx is done and acked a frame with the tx_info is
		 * sent to original radio iface
		*/
		acked++;
		int signal = get_signal_by_rate(tx_attempts[round-1].idx);
		/* Let's flag this frame as ACK'ed */
		flags |= HWSIM_TX_STAT_ACK;
		send_tx_info_frame_nl(src, flags, signal, tx_attempts, cookie);
	} else {
		send_tx_info_frame_nl(src, flags, 0, tx_attempts, cookie);
	}
}

/*
 * 	Callback function to process messages received from kernel
 */

static int process_messages_cb(struct nl_msg *msg, void *arg)
{

	struct nlattr *attrs[HWSIM_ATTR_MAX+1];
	/* netlink header */
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	/* generic netlink header*/
	struct genlmsghdr *gnlh = nlmsg_data(nlh);

	if (gnlh->cmd != HWSIM_CMD_FRAME)
		return 0;

	/* we get the attributes*/
	genlmsg_parse(nlh, 0, attrs, HWSIM_ATTR_MAX, NULL);
	if (!attrs[HWSIM_ATTR_ADDR_TRANSMITTER])
		return 0;

	struct mac_address *src = (struct mac_address*)
		nla_data(attrs[HWSIM_ATTR_ADDR_TRANSMITTER]);

	unsigned int data_len =
		nla_len(attrs[HWSIM_ATTR_FRAME]);
	char* data = (char*)nla_data(attrs[HWSIM_ATTR_FRAME]);
	unsigned int flags =
		nla_get_u32(attrs[HWSIM_ATTR_FLAGS]);
	printf("flags: %d\n", flags);
	struct hwsim_tx_rate *tx_rates =
		(struct hwsim_tx_rate*)
		nla_data(attrs[HWSIM_ATTR_TX_INFO]);
	unsigned long cookie = nla_get_u64(attrs[HWSIM_ATTR_COOKIE]);
	received++;

	printf("frame [%d] length:%d\n",received,data_len);

	send_msg_to_dispatcher(src, data, data_len, cookie);

	return 0;
}

/*
 * 	Send a register message to kernel
 */

int send_register_msg()
{

	msg = nlmsg_alloc();
	if (!msg) {
		printf("Error allocating new message MSG!\n");
		return -1;
	}

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, genl_family_get_id(family),
		    0, NLM_F_REQUEST, HWSIM_CMD_REGISTER, VERSION_NR);
	nl_send_auto_complete(sock,msg);
	nlmsg_free(msg);

	return 0;

}

/*
 *	Signal handler
 */
void kill_handler() {
	running = 0;
}



/*
 * 	Init netlink
 */

void init_netlink()
{

	cb = nl_cb_alloc(NL_CB_CUSTOM);
	if (!cb) {
		printf("Error allocating netlink callbacks\n");
		exit(EXIT_FAILURE);
	}

	sock = nl_socket_alloc_cb(cb);
	if (!sock) {
		printf("Error allocationg netlink socket\n");
		exit(EXIT_FAILURE);
	}

	genl_connect(sock);
	genl_ctrl_alloc_cache(sock, &cache);

	nl_socket_set_nonblocking(sock);
	nl_fd = nl_socket_get_fd(sock);
	family = genl_ctrl_search_by_name(cache, "MAC80211_HWSIM");

	if (!family) {
		printf("Family MAC80211_HWSIM not registered\n");
		exit(EXIT_FAILURE);
	}

	nl_cb_set(cb, NL_CB_MSG_IN, NL_CB_CUSTOM, process_messages_cb, NULL);

}

void init_dispatcher_fd(void)
{
	int ret;
	struct sockaddr_in sin;
	struct addrinfo *result, *res, hints;

	disp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (disp_fd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if ((ret = getaddrinfo(dispatcher, NULL, &hints, &result)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	memcpy(&sin, result->ai_addr, result->ai_addrlen);
	sin.sin_port = htons(port);
	if (connect(disp_fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);
}

int send_msg_to_dispatcher(struct mac_address *src, void *data, size_t data_len, unsigned long cookie)
{
	unsigned char buffer[2*1024];
	ssize_t ret;
	char addr[18];

	mac_address_to_string(addr, src);

	ret = fmt_msg(buffer, sizeof(buffer), cookie, addr);
	if (ret < 0) {
		perror("fmt_msg");
		return -1;
	}

	/* FIXME: check that we don't overflow the buffer */
	memcpy(buffer + ret, data, data_len);

	send(disp_fd, buffer, ret + data_len, 0);
	return 0;

}

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Get the message from the network and send the ACK */
int recv_and_ack(void)
{
	unsigned char buffer[2*1024];
	ssize_t recvd, to_send;
	struct wmd_msg msg;
	int signal;


	recvd = recv(disp_fd, buffer, sizeof(buffer), 0);
	if (recvd < 0) {
		perror("recv");
		return -1;
	}

	/* parse the data */
	if (parse_msg(&msg, buffer, recvd) < 0)
		return -1;

	signal = get_signal_by_rate(0); /* dummy */
	if (msg.ack) {
		/* FIXME: keep track of this in the protocol? */
		struct hwsim_tx_rate tx_attempts[IEEE80211_MAX_RATES_PER_TX];
		/* FIXME: figure out if we should set the rest of the flags */
		int flags = HWSIM_TX_STAT_ACK;
		struct mac_address addr = string_to_mac_address(msg.src);

		set_all_rates_invalid(tx_attempts);
		tx_attempts[0].count = 1;

		send_tx_info_frame_nl(&addr, flags, signal, tx_attempts, msg.cookie);
		/*
		 * flags |= HWSIM_TX_STAT_ACK;
		 * send_tx_info_frame_nl(src, flags, signal, tx_attempts, cookie);
		 */
		return 0;
	}

	/* send cloned frame to iface */
	send_cloned_frame_msg(&mac_addr, msg.data, msg.data_len, 0, signal);

	/* send back the ACK */
	to_send = fmt_ack(buffer, sizeof(buffer), msg.cookie, mac, msg.src);
	/* FIXME: check return value */
	send(disp_fd, buffer, to_send, 0);
	return 0;
}

void ping(void)
{
	char data[4 + 1 + 17];
	size_t len = sizeof(data);

	puts("PING");
	snprintf(data, len, "PING %s", mac);
	send(disp_fd, data, len, 0);
}

void main_loop(void)
{
	fd_set rfds;
	int nfds = MAX(nl_fd, disp_fd) + 1;
	int ret;
	struct timeval tv;

	while (running) {
		FD_ZERO(&rfds);
		FD_SET(nl_fd, &rfds);
		FD_SET(disp_fd, &rfds);

		/* Max sleep 0.5 secs */
		tv.tv_sec = 0;
		tv.tv_usec = 500000;

		/* TODO: see if it makes sense to prefer a different one each time */
		ret = select(nfds, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(disp_fd, &rfds)) {
			recv_and_ack();
		} else if (FD_ISSET(nl_fd, &rfds)) {
			/* receive nl messages until no more data is available */
			nl_recvmsgs_default(sock);
		} else {
			/* We haven't sent any messages in a while, send a ping to the dispatcher */
			ping();
		}
	}

	puts("exited main loop");
}

/*
 *	Print the CLI help
 */

void print_help(int exval)
{
	printf("wmediumd v%s - a wireless medium simulator\n", VERSION_STR);
	printf("wmediumd [-h] [-V] [-d <host>] [-p <port>]\n\n");

	printf("  -h              print this help and exit\n");
	printf("  -V              print version and exit\n\n");
	printf("  -d              dispatcher to connect to\n");
	printf("  -p              dispatcher's port (defaults to 5555)\n");
	printf("  -a              address to use as source\n");

	exit(exval);
}


int main(int argc, char* argv[]) {

	int opt, ifaces;

	/* Set stdout buffering to line mode */
	setvbuf (stdout, NULL, _IOLBF, BUFSIZ);

	/* no arguments given */
	if(argc == 1) {
		fprintf(stderr, "This program needs arguments....\n\n");
		print_help(EXIT_FAILURE);
	}

	while((opt = getopt(argc, argv, "hVd:p:a:b")) != -1) {
		switch(opt) {
		case 'h':
			print_help(EXIT_SUCCESS);
    			break;
		case 'V':
			printf("wmediumd v%s - a wireless medium simulator "
			       "for mac80211_hwsim\n", VERSION_STR);
			exit(EXIT_SUCCESS);
			break;
		case 'd':
			dispatcher = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'a':
			mac = strdup(optarg);
			break;
		case 'b': /* background (daemonize) */
			background = 1;
			break;
		case '?':
			printf("wmediumd: Error - No such option:"
			       " `%c'\n\n", optopt);
			print_help(EXIT_FAILURE);
			break;
		}

	}

	if (optind < argc)
		print_help(EXIT_FAILURE);


	/*Handle kill signals*/
	running = 1;
	signal(SIGUSR1, kill_handler);

	/*init netlink*/
	init_netlink();

	init_dispatcher_fd();

	/*Send a register msg to the kernel*/
	if (send_register_msg()==0)
		printf("REGISTER SENT!\n");

	if (background && daemon(0, 0)) {
		perror("daemon");
		return EXIT_FAILURE;
	}

	main_loop();
	
	/*Free all memory*/
	free(sock);
	free(msg);
	free(cb);
	free(cache);
	free(family);

	return EXIT_SUCCESS;
}
