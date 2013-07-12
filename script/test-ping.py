#!/usr/bin/env python

import subprocess, shlex
import signal, time
import os, sys
from optparse import OptionParser

parser = OptionParser(usage="%prog [options] host")
parser.add_option("-t", "--time", dest="time", type="int", help="How long the test should run (seconds)", default=15)
parser.add_option("-f", "--flood", action="store_true", dest="flood", help="Activate ping flooding")
parser.add_option("-d", "--destination", dest="destination", help="What host to ping")
(options, args) = parser.parse_args()

if len(args) != 1:
    parser.error("No host specified")

if options.flood and os.geteuid() != 0:
    print >> sys.stderr, "Flooding disabled for non-root users"
    exit(1)

# -q simplifies parsing a bit
floodstr = "-f" if options.flood else ""
pingstr = "ping %s -q %s" % (floodstr, args[0])
ping = subprocess.Popen(shlex.split(pingstr), stdout=subprocess.PIPE)

time.sleep(options.time)
ping.send_signal(signal.SIGINT)

# This gets the "X packets transmitted, Y received..." line
line = ping.stdout.read().strip().split('\n')[-2]

# Assuming that the output of ping isn't going to change any time soon
parts = line.split()
sent = int(parts[0])
received = int(parts[3])
loss = 1 - (received/sent)

print("Loss %f" % loss)
