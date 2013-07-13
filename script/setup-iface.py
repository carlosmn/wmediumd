#!/usr/bin/env python

import re, shlex
from subprocess import call

def run(str):
    call(shlex.split(str))

IFACE = "mesh0"

cmdline = open("/proc/cmdline").read()

pattern = re.compile(r'.*wmediumd\.id=(.*)\s')
id = pattern.match(cmdline).group(1)

# Start by adding an IP address to eth0 so we can talk to the
# dispatcher
run("ip address add 172.16.0.%s/12 dev eth0" % id)
run("ip link set eth0 up")

# Load the kernel module
run("modprobe mac80211")
insmod = "insmod /wmediumd/mac80211_hwsim.ko radios=1 mac_offset=%s" % id
run(insmod)

# Create the interface over which the nodes will talk to each other,
# join it to "testnet" with a particular BSSID and give it its IP
# address for the wireless network
add_iface = "iw dev wlan0 interface add %s type ibss" % IFACE
run(add_iface)

iface_up = "ip link set %s up" % IFACE
run(iface_up)

join_net = "iw dev %s ibss join testnet 2412 26:1C:41:D1:AC:90" % IFACE
run(join_net)

set_ip = "ip address add 192.168.2.%s/24 dev %s" % (id, IFACE)
run(set_ip)
