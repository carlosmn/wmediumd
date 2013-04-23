#ifndef _WMEDIUMD_PROTO_H
#define _WMEDIUMD_PROTO_H

#include "mac_address.h"
#include <stdlib.h>

struct wmd_msg {
	char src[18];
	int ack :1;
	unsigned long cookie;
	const char *data;
	size_t data_len;
};

int fmt_msg(unsigned char *buf, size_t sz, unsigned long cookie, char *src);
int fmt_ack(unsigned char *buf, size_t sz, unsigned long cookie, char *src, char *dst);
int parse_msg(struct wmd_msg *out, const unsigned char *buf, size_t sz);

#endif /* _WMEDIUMD_PROTO_H */
