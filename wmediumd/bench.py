#!/usr/bin/env python

import subprocess
import signal
import time

clients = []
dispatcher = None
bench_time = 15

dispatcher = subprocess.Popen(["./dispatcher"])
print("Giving the dispatcher time to set up")
time.sleep(1)

for i in range(4):
    mac = "42:00:00:00:%02d:00" % i # wmediumd uses decimal for some reason
    print("Starting test for %s" % mac)
    clients.append(subprocess.Popen(["./test", "-t", "1", "-s", mac], stdout=subprocess.PIPE))

time.sleep(bench_time)

for client in clients:
    client.send_signal(signal.SIGINT)

dispatcher.send_signal(signal.SIGINT);

aggregate = 0
for client in clients:
    str = client.stdout.read().strip()
    (amt, time, mac) = str.split()
    time = int(time)
    amt = int(amt)

    aggregate += amt

    print("Client %s %d @ %d/s" % (mac, amt, amt/time))

aggregate = (aggregate / bench_time)
print("Aggregate speed %f pkts/s" % (aggregate))
