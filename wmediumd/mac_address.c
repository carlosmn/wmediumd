/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author: Javier Lopez	<jlopex@cozybit.com>
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

#include <stdio.h>
#include "mac_address.h"

struct mac_address string_to_mac_address(const char* str)
{
	struct mac_address mac;
	int a[6];

	sscanf(str, "%x:%x:%x:%x:%x:%x", 
	       &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);

	mac.addr[0] = a[0];
	mac.addr[1] = a[1];
	mac.addr[2] = a[2];
	mac.addr[3] = a[3];
	mac.addr[4] = a[4];
	mac.addr[5] = a[5];

	return mac;
}

static char hexen[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void mac_address_to_string(char *str, const struct mac_address *mac)
{
	int i, off;
	for (i = 0, off = 0; i < 6; i++) {
		str[i*2+off] = hexen[(mac->addr[i] & 0xf0) >> 4];
		str[i*2+off+1] = hexen[mac->addr[i] & 0x0f];
		str[i*2+off+2] = ':';
		off += 1;
	}

	/* The loop above write a colon too much, compensate */
	str[(i-1)*2+off+1] = '\0';
}
