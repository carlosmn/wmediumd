#!/usr/bin/env python

import socket, sys
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-n", "--number", dest="number", type="int", help="Number of messages to send")
(options, args) = parser.parse_args()

if len(args) != 1:
    parser.error("No host specified")

target = args[0]

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

for i in range(options.number):
    s.sendto("PING", (target, 1337))
    print "%d\r" % i,
    sys.stdout.flush()

print ""
