Setting up Nvidia Jetson TK1

Useful links and references:

- https://developer.nvidia.com/jetson-tk1
- http://elinux.org/Jetson_TK1
- https://devtalk.nvidia.com/default/topic/785551/embedded-systems/my-jetson-focused-linux-tips-and-tricks/
- https://cyclicredundancy.wordpress.com/2014/05/10/flashing-the-rootfs-on-a-nvidia-jetson-tk1/

Connect keyboard, mouse, monitor

Connect power to turn on tk1

Open terminal ctrl-alt-t

Verify that start stop restart works

    $   sudo poweroff
    $   sudo reboot

got internet?

    $   sudo ping google.com

setup timezone

    $   sudo dpkg-reconfigure tzdata

set sysclock, only if internet is down

    $   sudo date mmddhhmmyyyy.ss
    $   sudo date 050607002014

check time on HW RTC

    $   sudo hwclock --debug

sync HW RTC to sysclock

    $   sudo hwclock -w    

add automatic time update at startup and to crontab

    $   sudo vi /etc/rc.local
    
add the following lines

    ntpdate-debian
    hwclock -w
    
edit crontab for root

    $   sudo crontab -e
    
add following lines

    5 * * * * ntpdate-debian
    7 * * * * hwclock -w    

enable universe in the apt source list

    $   sudo vi /etc/apt/sources.list

"libglx.so" is a specific file in NVIDIA's graphics driver that might get replaced by an incorrect version when doing apt-get, so we hold it. (this may be old?)
    
    $   sudo apt-mark hold xserver-xorg-core

update and upgrade (This will take a while)

    $   sudo apt-get update && sudo apt-get upgrade

install useful packages

    $   sudo apt-get install git screen tee build-essential cmake

misc cleanup

    $   sudo apt-get autoclean && sudo apt-get clean

disable gui desktop and window manager
from here: http://www.pathbreak.com/blog/ubuntu-startup-init-scripts-runlevels-upstart-jobs-explained    

    $   sudo mv /etc/init/lightdm.conf /etc/init/lightdm.conf.disabled

Auto login user "ubuntu" on first console terminal (tty1) 

    $   sudo vim /etc/init/tty1.conf

replace last line:

    exec /sbin/getty -8 38400 tty1

with this:

    exec /bin/login -f bob < /dev/tty1 > /dev/tty1 2>&1    


