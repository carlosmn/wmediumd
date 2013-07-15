#!/usr/bin/env python

import socket
from select import select
from signal import signal, SIGINT
from sys import exit

recvd = 0

def handle_sigint(signum, frame):
    print("Received %d" % recvd)
    exit(0)

signal(SIGINT, handle_sigint)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("0.0.0.0", 1337))

while True:
    data, addr = s.recvfrom(8)
    recvd += 1
