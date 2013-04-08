#!/usr/bin/env python

import subprocess
import signal
import time

clients = []
dispatcher = None
bench_time = 15
num_clients = 6
loss_prob = 0.0

# Write out the configuration for the dispatcher

write_config = subprocess.Popen(["./write-config", "-l", str(loss_prob), "-n", str(num_clients),
                                 "-o", "dispatcher-config.cfg"], stdout=subprocess.PIPE)
write_config.communicate() # wait for it to finish


dispatcher = subprocess.Popen(["./dispatcher"])
print("Giving the dispatcher time to set up")
time.sleep(1)


for i in range(num_clients):
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
