#ifndef _WMEDIUMD_PROTO_H
#define _WMEDIUMD_PROTO_H

struct wmd_msg {
	const char *src;
	const char *cookie;
	const char *data;
};

int fmt_msg(unsigned char *buf, size_t sz, unsigned long cookie, char *src);
int fmt_ack(unsigned char *buf, size_t sz, unsigned long cookie, char *src, char *dst);

#endif /* _WMEDIUMD_PROTO_H */
