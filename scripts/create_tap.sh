#!/bin/bash

# This script creates a tap network interface and adds it to a bridge

USER=$(whoami)
TAP=tap0
IP=192.168.0.95/24
BRIDGE=virbr0

echo "Setting TAP interface for user $USER"
if ip link show | grep -q "$TAP"; then
    echo "The tap device $TAP already exists"
    exit 0
fi

# Check if the bridge exists, and then create it it doesn't
if ! ip link show | grep -q "$BRIDGE"; then
    sudo ip link add name "$BRIDGE" type bridge
    sudo ip link set dev "$BRIDGE" up
fi
    
sudo ip tuntap add dev "$TAP" mode tap user "$USER"
sudo ip addr add "$IP" dev "$TAP"
sudo ip link set dev "$TAP" up

sudo ip link set dev "$TAP" master "$BRIDGE"