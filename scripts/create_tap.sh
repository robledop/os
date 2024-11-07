#!/bin/bash

#USER=$(whoami)
USER=robledo
TAP=tap0
IP=192.168.0.95/24
BRIDGE=br0

sudo ip tuntap add dev "$TAP" mode tap user "$USER"
sudo ip addr add "$IP" dev "$TAP"
sudo ip link set dev "$TAP" up

sudo ip link set dev "$TAP" master "$BRIDGE"