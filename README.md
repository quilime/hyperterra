# Hyper Terra

"Hyper Terra" is a series of projection sculptures by [Gabriel Dunne](http://gabrieldunne.com)

![https://farm8.staticflickr.com/7558/16110524908_6d91e1e6df_c.jpg](https://farm8.staticflickr.com/7558/16110524908_6d91e1e6df_c.jpg)

## Software Abstract

OpenGL projection graphics software coded in C++ using a linux dev branch of the [Cinder](http://libcinder.org/) framework (included as a submodule). 

3D graphics depict an abstract landscape that can exist in any time and lat/long on Earth. Lights are position at the Sun and Moon positions of that time and place, calculated by an [ephemeris](http://en.wikipedia.org/wiki/Ephemeris) table, via pyephem, a library for Python.

## Screenshots

![https://farm4.staticflickr.com/3874/14972519720_96154ba3dd_c.jpg](https://farm4.staticflickr.com/3874/14972519720_96154ba3dd_c.jpg)


# Hardware

- Computer: Nvidia Jetson TK1 
- OS: Linux/Ubuntu 14.04 OS
- Projector: PowerLite 1730W LCD Multimedia Projector (1280 x 800 native)

# Software

## Installation: Ubuntu 14.04 / Arm

First, follow the TK1 setup in README.Jetson-TK1.md, or this [gist](https://gist.github.com/quilime/0104aa2268cd8e5f0a51)

Install Python and Python-dev

    # install python dev tools
    $   sudo apt-get install python-dev python-pip
    # install py ephem
    $   sudo pip install pyephem               

Install deps for Cinder

    $   sudo apt-get update
    $   sudo apt-get install build-essential cmake libgl1-mesa-dev libxrandr-dev libxi-dev libxcursor-dev libfreeimage-dev libpng12-dev libglew-dev libboost-system1.55.0 libboost-system1.55-dev libboost-filesystem1.55.0 libboost-filesystem1.55-dev libpthread-stubs0-dev zlib1g-dev

Clone source and build executable

    $   git clone https://github.com/quilime/hyperterra.git
    $   cd hyperterra
    $   git submodule update --init --recursive
    $   export CINDER_ROOT=/home/ubuntu/hyperterra/deps/Cinder
    $   cd deps/Cinder 
    $   make release
    $   cd ../../
    $   make

Soft-link hyperterra/ephemScript.py to local python lib location so it's available globally.

    $   sudo ln -s /home/ubuntu/hyperterra/assets/ephemScript.py /usr/local/lib/python2.7/dist-packages/
    
Run

    $   ./linux/bin/release/LandscapeApp
    
Debugging
    
    $   make debug
    $   gdb ./linux/debug/bin/LandscapeApp_D # once in gdb, type 'r' to run

## OSX 10.10

[TDB]
    
    $   ln -s hyperterra/assets/ephemScript.py /Library/Python/2.7/site-packages/


    
## Usage

Controlled by Liine Lemur App
