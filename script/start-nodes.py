#!/usr/bin/env python

# Start a number of nodes which all talk to the same dispatcher

import os
import subprocess, shlex, signal
from optparse import OptionParser

BASE_PATH = "/home/carlos/slow/carlos/machines/"
BASE_IMAGE = BASE_PATH + "wmediumd-skel"
KERNEL = BASE_PATH + "vmlinuz-3.8-2-amd64"
INITRD = BASE_PATH + "initrd.img-3.8-2-amd64"
OPTIONS = "root=UUID=a9468c72-4a4a-4463-aa25-5596f9b951b7 ro quiet"

dispatcher = 0

def start_one(n): 
    wmd_opts = "wmediumd.dispatcher=192.168.2.1 wmediumd.id=%d" % n
    net_opts = "-netdev type=tap,id=tap%02x,vhost=on,script=wmediumd-ifup -device virtio-net-pci,netdev=tap%02x,mac=52:54:00:00:%02x:00"
    net_opts = net_opts % (n, n, n)
    cmdbase = "qemu-system-x86_64 -enable-kvm %s -snapshot %s -kernel %s -initrd %s -append '%s %s'"
    cmdline = cmdbase % (net_opts, BASE_IMAGE, KERNEL, INITRD, OPTIONS, wmd_opts)

    command = shlex.split(cmdline)
    return subprocess.Popen(command)

def rewrite_config(loss):
    cmd = shlex.split("../wmediumd/write-config -n 50 -o dispatcher-config.cfg -l %s" % loss)
    subprocess.call(cmd, stdout=None, stderr=None)

def run(str):
    subprocess.call(shlex.split(str))

## Program start

parser = OptionParser()
parser.add_option("-n", "--nodes", dest="nodes", type="int", help="Number of nodes")
(options, args) = parser.parse_args()

# Create the bridge we'll add the VM interfaces to. The bridge needs
# to have the address assigned to it so the kernel knows how to route
run("brctl addbr br-wmediumd")
run("ip address add 172.16.0.1/12 dev br-wmediumd")
run("ip link set br-wmediumd up")

# We start the nodes at 2 so the addresses and MACs line up and we can
# have the .1 IP address for the dispatcher
nodes = [start_one(i+2) for i in range(options.nodes)]

try:
    while True:
        command = raw_input("> ")
        if command.startswith("loss"):
            rewrite_config(command[5:])
            dispatcher.send_signal(signal.SIGHUP)
        else:
            print("Unkown command")
except Exception as e:
    print e

# Bring down the bridge
run("ip link set br-wmediumd down")
run("brctl delbr br-wmediumd")

# And kill the virtual machines
for node in nodes:
    node.send_signal(signal.SIGINT)

dispatcher.send_signal(signal.SIGINT)
