#include "proto.h"

int fmt_msg(unsigned char *buf, size_t sz, unsigned long cookie, char *src)
{
	return snprintf(buf, sz, "MSG %lu %s ", cookie, mac);
}

int fmt_ack(unsigned char *buf, size_t sz, unsigned long cookie, char *src, char *dst)
{
	return snprintf(buf, sz, "ACK %lu %s %s", cookie, src, dst);
}

int parse_msg(wmd_msg *out, const unsigned char *buf, size_t sz)
{
	const char *pfx = "MSG ", *ptr = buf;
	size_t pfx_len = strlen(pfx);

	if (sz < pfx_len)
		return -1;

	
}
