#!/bin/sh

sudo systemctl stop a36-display
sudo systemctl disable a36-display
sudo rm /usr/bin/a36-display
