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

int parse_msg(struct wmd_msg *out, const unsigned char *buf, size_t sz)
{
	const char *pfx = "MSG ", *ptr = buf;
	char *endptr;
	size_t pfx_len = strlen(pfx);
	size_t mac_len = 17; /* string length of a formatted MAC address */

	memset(out, 0, sizeof(struct wmd_msg));

	if (sz < pfx_len)
		return -1;

	if (memcmp(buf, pfx, pfx_len))
		return -1;

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

	out->data = ptr;
	out->data_len = sz;

	return 0;
}
