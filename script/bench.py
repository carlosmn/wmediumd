#!/usr/bin/env python

import subprocess
import signal
import time

class Client:
    popen = None
    mac = ""
    sent = 0
    recvd = 0
    recvd_exp = 0

dispatcher = None
bench_time = 15
num_clients = 5
loss_prob = 0.0

clients = []

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
    client = Client()
    client.mac = mac
    client.popen = subprocess.Popen(["./test", "-t", "1", "-s", mac], stdout=subprocess.PIPE)
    clients.append(client)

time.sleep(bench_time)

for client in clients:
    client.popen.send_signal(signal.SIGINT)

dispatcher.send_signal(signal.SIGINT);

aggregate = 0
for i in range(len(clients)):
    client = clients[i]
    str = client.popen.stdout.read().strip()
    (sent, recvd, time, mac) = str.split()
    time = int(time)
    amt = int(recvd)

    client.sent = int(sent)
    client.recvd = int(recvd)
    aggregate += amt

    print("Client %s %d @ %d/s" % (mac, amt, amt/time))

    # With 0 loss, this is how many each should have received
    for j in range(len(clients)):
        if i == j: continue
        clients[j].recvd_exp += int(sent)

aggregate = (aggregate / bench_time)
print("Aggregate speed %f pkts/s" % (aggregate))

avg = 0.0
for client in clients:
    ratio = float(client.recvd)/float(client.recvd_exp)
    avg += ratio
    print("%s sent %d recvd %d out of %d (%f)" %
          (client.mac, client.sent, client.recvd, client.recvd_exp, ratio))

avg /= len(clients)
print("Average %f" % avg)
