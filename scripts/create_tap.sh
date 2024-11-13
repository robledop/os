#!/bin/bash

# This script creates a tap network interface and adds it to a bridge

USER=$(whoami)
TAP=tap0
IP=192.168.0.95/24
BRIDGE=br0

if ip link show | grep -q "$TAP"; then
    echo "The tap device $TAP already exists"
    exit 0
fi

sudo ip tuntap add dev "$TAP" mode tap user "$USER"
sudo ip addr add "$IP" dev "$TAP"
sudo ip link set dev "$TAP" up

sudo ip link set dev "$TAP" master "$BRIDGE"