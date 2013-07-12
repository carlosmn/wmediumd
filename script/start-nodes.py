#!/usr/bin/env python

# Start a number of nodes which all talk to the same dispatcher

import subprocess
import shlex
from optparse import OptionParser

BASE_IMAGE = "wmediumd-skel"
KERNEL = "vmlinuz-3.8-2-amd64"
INITRD = "initrd.img-3.8-2-amd64"
OPTIONS = "root=UUID=a9468c72-4a4a-4463-aa25-5596f9b951b7 ro quiet"

def start_one(n): 
    wmd_opts = "wmediumd.dispatcher=192.168.2.1 wmediumd.id=%d" % n
    net_opts = "-netdev type=tap,id=tap%02x,vhost=on -device virtio-net-pci,netdev=tap%02x,mac=52:54:00:00:%02x:00"
    net_opts = net_opts % (n, n, n)
    cmdbase = "qemu-system-x86_64 -enable-kvm %s -snapshot %s -kernel %s -initrd %s -append '%s %s'"
    cmdline = cmdbase % (net_opts, BASE_IMAGE, KERNEL, INITRD, OPTIONS, wmd_opts)

    command = shlex.split(cmdline)
    return subprocess.Popen(command)

## Program start

parser = OptionParser()
parser.add_option("-n", "--nodes", dest="nodes", type="int", help="Number of nodes")
(options, args) = parser.parse_args()

# We start the nodes at 2 so the addresses and MACs line up and we can
# have the .1 IP address for the dispatcher
nodes = [start_one(i+2) for i in range(options.nodes)]

print nodes
