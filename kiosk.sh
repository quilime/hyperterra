#!/bin/bash

# this file assumes it is in /opt/kiosk.sh
# runs application with screensaver and screen-blanking disabled

xset -dpms
xset s off
/home/ubuntu/hyperterra/linux/release/bin/LandscapeApp
