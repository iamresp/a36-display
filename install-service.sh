#!/bin/sh

ln -s $PWD/dist/a36-display /usr/bin/a36-display
ln -s $PWD/a36-display.service /etc/systemd/system/a36-display.service
systemctl enable a36-display
systemctl start a36-display
