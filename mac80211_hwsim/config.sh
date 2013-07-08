#!/bin/sh

#sudo ip link set up wlan0
#sudo ip link set up wlan1

sudo iw dev wlan0 interface add mesh0 type ibss
sudo iw dev wlan1 interface add mesh1 type ibss

sudo ip link set mesh0 up
sudo ip link set mesh1 up

sudo iw dev mesh0 ibss join testnet 2412
sudo iw dev mesh1 ibss join testnet 2412

sudo ip a add 192.168.2.10/24 dev mesh0
sudo ip a add 192.168.2.20/24 dev mesh1
