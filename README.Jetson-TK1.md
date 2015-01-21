Basic setup guide for Nvidia Jetson TK1

Links and references:

- https://developer.nvidia.com/jetson-tk1
- http://elinux.org/Jetson_TK1
- https://devtalk.nvidia.com/default/topic/785551/embedded-systems/my-jetson-focused-linux-tips-and-tricks/
- https://cyclicredundancy.wordpress.com/2014/05/10/flashing-the-rootfs-on-a-nvidia-jetson-tk1/


## First Steps

Connect keyboard, mouse, monitor

Connect power to turn on TK1

Login with default 'ubuntu' user, password: 'ubuntu'

Open terminal via ctrl-alt-t

Verify that poweroff and reboot works

    $   sudo poweroff
    $   sudo reboot

Ping

    $   sudo ping google.com

Enable universe in the apt source list

    $   sudo vi /etc/apt/sources.list

"libglx.so" is a specific file in NVIDIA's graphics driver that might get replaced by an incorrect version when doing apt-get, so we hold it. (this may be old?)
    
    $   sudo apt-mark hold xserver-xorg-core

You can also hold libglx.so by making a backup

    $   sudo cp /usr/lib/xorg/modules/extensions/libglx.so /usr/lib/xorg/modules/extensions/libglx.so-19r3

It takes a quite a while to login with ssh as Ubuntu checks for updates. To disable that, edit

    /etc/update-motd.d/90-updates-available
    /etc/update-motd.d/91-release-upgrade    

Update and upgrade (This may take a while)

    $   sudo apt-get update && sudo apt-get upgrade

install packages

    $   sudo apt-get install tee git screen lshw cmake build-essential linux-firmware linux-headers-generic

misc cleanup

    $   sudo apt-get autoclean && sudo apt-get clean



## Setting The Clock

Setup timezone

    $   sudo dpkg-reconfigure tzdata

Set sysclock

    $   sudo date mmddhhmmyyyy.ss
    $   sudo date 050607002014

Check time on Hardware real-time clock

    $   sudo hwclock --debug

Sync HW RTC to sysclock

    $   sudo hwclock -w    

To check time on startup, add automatic time update at startup and to crontab

    $   sudo vi /etc/rc.local
    
add the following lines

    ntpdate-debian
    hwclock -w
    
For repeated checking, edit crontab for root:

    $   sudo crontab -e
    
Add following lines:

    5 * * * * ntpdate-debian
    7 * * * * hwclock -w    



## Networking

Scan for USB network devices

    $   sudo lshw -C network

    and/or

    $   lspci -nnk | grep -i net -A2





## Set up as Kiosk

References:

- http://thepcspy.com/read/building-a-kiosk-computer-ubuntu-1404-chrome/
- http://thepcspy.com/read/converting-ubuntu-desktop-to-kiosk/
- http://askubuntu.com/questions/509330/execute-single-program-on-boot-no-menus


    $   sudo apt install --no-install-recommends openbox
    $   sudo install -b -m 755 /dev/stdin /opt/kiosk.sh << EOF
    #!/bin/bash

    xset -dpms
    xset s off
    openbox-session &

    while true; do
        /home/ubuntu/hyperterra/linux/release/bin/LandscapeApp
    done
    EOF

    $   sudo install -b -m 644 /dev/stdin /etc/init/kiosk.conf << EOF
    start on (filesystem and stopped udevtrigger)
    stop on runlevel [06]

    emits starting-x
    respawn

    exec sudo -u $USER startx /etc/X11/Xsession /opt/kiosk.sh --
    EOF

    $   sudo dpkg-reconfigure x11-common  # select 'Anybody' in menu popup to allow any user to launch X11

    $   echo manual | sudo tee /etc/init/lightdm.override  # disable desktop window manager
    # or
    $   sudo mv /etc/init/lightdm.conf /etc/init/lightdm.conf.disabled

    $   sudo reboot


Access system settings from black xsession terminal. Use `ctrl-alt-t` to open a terminal from blank xsession

    $   unity-control-center

To login automatically to the graphical environment:

    sudo mkdir -p /etc/lightdm/lightdm.conf.d
    sudo vim /etc/lightdm/lightdm.conf.d/autologin.conf

And add the following lines:

    [SeatDefaults]
    autologin-user=ubuntu

Another way to auto login user "ubuntu" on first console terminal (tty1) :

    $   sudo vim /etc/init/tty1.conf

replace last line:

    exec /sbin/getty -8 38400 tty1

with:

    exec /bin/login -f bob < /dev/tty1 > /dev/tty1 2>&1    

Change the default window manager (works also with the autologin):

    sudo mkdir -p /etc/lightdm/lightdm.conf.d
    sudo vim /etc/lightdm/lightdm.conf.d/xfce.conf

    And add the following lines:

    [SeatDefaults]
    user-session=xfce



## VNC

Install VNC server:

    sudo apt-get install tightvncserver

Confirmed that /etc/ssh/ssh_config on the server contains the lines

    ForwardAgent yes
    ForwardX11 yes
    ForwardX11Trusted yes

