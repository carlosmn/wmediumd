#!/usr/bin/env python

import subprocess, re

# Load the kernel module
res = subprocess.call(["modprobe", "mac80211_hwsim", "radios=1"])
if res != 0:
    print("boo!")

cmdline = open("/proc/cmdline").read()

# Match the wmediumd.iface=wlan1 option
pattern = re.compile(r'.*wmediumd\.iface=(.*)\s')
iface = pattern.match(cmdline).group(1)

# Match the wmediumd.addr
pattern = re.compile(r'.*wmediumd\.addr=(.*)\s')
addr = pattern.match(cmdline).group(1)

# Match the dispatcher's address
pattern = re.compile(r'.*wmediumd\.dispatcher=(.*)\s')
disp = pattern.match(cmdline).group(1)

# Set the MAC address for our virtual wireless interface
res = subprocess.call(["ip", "link", "set", "dev", iface, "address", addr])
if res != 0:
    print("Error setting the MAC address")

res = subprocess.call(["ip", "link", "set", iface, "up"])
if res != 0:
    print("Error setting the MAC address")

# This part we can probably let the user do when they need to
res = subprocess.call(["iw", "dev", iface, "set", "type", "ibss"])
if res != 0:
    print("Error setting IBSS mode")

res = subprocess.call(["iw", "dev", iface, "ibss", "join", "testnet", "2412"])
if res != 0:
    print("Error joining testnet")

# Now we can start wmediumd

subprocess.call(["wmediumd", "-d", disp, "-a", addr, "-b"])
if res != 0:
    print("Error starting wmediumd")

