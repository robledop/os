#!/bin/bash

# This script creates a tap network interface and adds it to a bridge

USER=$(whoami)
TAP=tap0
IP=192.168.0.95/24
BRIDGE=osdev

echo "Setting TAP interface for user $USER"
if ip link show | grep -q "$TAP"; then
    echo "The tap device $TAP already exists"
else 
    echo "Creating tap device $TAP"
    sudo ip tuntap add dev "$TAP" mode tap user "$USER"
    sudo ip addr add "$IP" dev "$TAP"
    sudo ip link set dev "$TAP" up
fi 

if ! ip link show | grep -q "$BRIDGE"; then
    sudo ip link add name "$BRIDGE" type bridge
    sudo ip link set dev "$BRIDGE" up
fi

# This is for my specific network interface, you may need to change this
sudo ip link set dev enp9s0 master "$BRIDGE"

sudo ip link set dev "$TAP" master "$BRIDGE"