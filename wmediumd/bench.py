#!/usr/bin/env python

import subprocess
import signal
import time

clients = []
dispatcher = None

dispatcher = subprocess.Popen(["./dispatcher"])
print("Giving the dispatcher time to set up")
time.sleep(1)

for i in range(10):
    mac = "42:00:00:00:0%02d:00" % i
    print("Starting test for %s" % mac)
    clients.append(subprocess.Popen(["./test", "-t", "0", "-s", mac]))

time.sleep(7)

for client in clients:
    client.send_signal(signal.SIGINT)

dispatcher.kill()
