#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "proto.h"

int fmt_msg(unsigned char *buf, size_t sz, unsigned long cookie, char *src)
{
	return snprintf(buf, sz, "MSG %lu %s ", cookie, src);
}

int fmt_ack(unsigned char *buf, size_t sz, unsigned long cookie, char *src, char *dst)
{
	return snprintf(buf, sz, "ACK %lu %s %s", cookie, src, dst);
}

int parse_msg_head(struct wmd_msg *out, const unsigned char *buf, size_t sz,
		   const char **ptr_out, size_t *sz_out)
{
	const char *pfx_msg = "MSG ", *pfx_ack = "ACK ";
	const char *ptr = buf;
	char *endptr;
	size_t mac_len = 17; /* string length of a formatted MAC address */
	size_t pfx_len = 4;

	if (sz < pfx_len)
		return -1;

	sz -= pfx_len;
	ptr += pfx_len;

	if (memcmp(buf, pfx_msg, pfx_len)) {
		if (memcmp(buf, pfx_ack, pfx_len)) {
			return -1;
		} else {
			out->ack = 1;
		}
	}

	sz -= pfx_len;
	ptr += pfx_len;

	errno = 0;
	out->cookie = strtoul(ptr, &endptr, 10);

	if (errno) /* only real way to check, according to stroul(3) */
		return -1;

	/* skip the SP */
	sz -= (endptr + 1 - ptr);
	ptr = endptr + 1;

	memcpy(out->src, ptr, mac_len);

	/* skip the SP */
	sz -= mac_len + 1;
	ptr += mac_len + 1;

	*ptr_out = ptr;
	*sz_out = sz;

	return 0;
}

int parse_msg(struct wmd_msg *out, const unsigned char *buf, size_t sz)
{
	const char *pfx = "MSG ", *ptr = buf;
	const char *ptr_out;
	size_t sz_out;

	memset(out, 0, sizeof(struct wmd_msg));

	/* Try to parse a message first */
	if (parse_msg_head(out, buf, sz, &ptr_out, &sz_out) < 0)
		return -1;


	if (out->ack)
		return 0;

	out->data = ptr_out;
	out->data_len = sz_out;

	return 0;
}
