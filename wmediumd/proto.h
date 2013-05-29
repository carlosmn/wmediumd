#ifndef _WMEDIUMD_PROTO_H
#define _WMEDIUMD_PROTO_H

#include "mac_address.h"
#include <stdlib.h>

struct wmd_msg {
	unsigned int ack :1,
		ping :1;
	char addr[18];
	unsigned long cookie;
	/* Following two only for MSG */
	const char *data;
	size_t data_len;
};

int fmt_msg(unsigned char *buf, size_t sz, unsigned long cookie, char *src);
int fmt_ack(unsigned char *buf, size_t sz, unsigned long cookie, char *dst);
int parse_msg(struct wmd_msg *out, const unsigned char *buf, size_t sz);

#endif /* _WMEDIUMD_PROTO_H */
