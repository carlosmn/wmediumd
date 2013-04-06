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
    clients.append(subprocess.Popen(["./test", "-t", "0", "-s", mac], stdout=subprocess.PIPE))

time.sleep(bench_time)

for client in clients:
    client.send_signal(signal.SIGINT)

dispatcher.kill()

aggregate = 0
for client in clients:
    str = client.stdout.read().strip()
    (amt, time, mac) = str.split()
    time = int(time)
    amt = int(amt)

    aggregate += amt

    print("Client %s @ %d" % (mac, amt/time))

aggregate_mb = (aggregate / bench_time) / (1024.0*1024.0)
print("Aggregate speed %f MB/s (%f Mbps)" % (aggregate_mb, aggregate_mb * 8))
