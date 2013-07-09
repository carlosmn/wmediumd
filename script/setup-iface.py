#!/usr/bin/env python

import re, shlex
from subprocess import call

IFACE = "mesh0"

cmdline = open("/proc/cmdline").read()

pattern = re.compile(r'.*wmediumd\.id=(.*)\s')
id = pattern.match(cmdline).group(1)

add_iface = "iw dev wlan0 interface add %s type ibss" % IFACE
call(shlex.split(add_iface))

# Load the kernel module
call(shlex.split("modprobe mac80211"))
insmod = "insmod /wmediumd/mac80211_hwsim.ko radios=1 mac_offset=%s" % id
call(shlex.split(insmod))

# The address should already be fine, and we can't change it anyway
#set_mac = "ip link set dev %s address 42:00:00:00:%02x:00" % IFACE, int(id)
#call(shlex.split(set_mac))

iface_up = "ip link set %s up" % IFACE
call(shlex.split(iface_up))

join_net = "iw dev %s ibss join testnet 2412 26:1C:41:D1:AC:90" % IFACE
call(shlex.split(join_net))

set_ip = "ip address add 192.168.2.%s/24 dev %s" % (id, IFACE)
call(shlex.split(join_net))
