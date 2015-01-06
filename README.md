# Hyper Terra

## Software Abstract

OpenGL projection graphics software coded in C++ using a linux dev branch of the [Cinder](http://libcinder.org/) framework (included as a submodule). 

"Hyper Terra" is a series of projection sculptures by [Gabriel Dunne](http://gabrieldunne.com)

3D graphics depic an abstract landscape that can exist in any time and lat/long on Earth. Lights are position at the Sun and Moon positions of that time and place, calculated by an [ephemeris](http://en.wikipedia.org/wiki/Ephemeris) table, via pyephem, a library for Python.

## Screenshots

![https://farm4.staticflickr.com/3874/14972519720_96154ba3dd_c.jpg](https://farm4.staticflickr.com/3874/14972519720_96154ba3dd_c.jpg)

# Hardware

- Computer: Nvidia Jetson TK1 
- OS: Linux/Ubuntu 14.04 OS
- Projector: PowerLite 1730W LCD Multimedia Projector (1280 x 800 native)

# Software

## Ubuntu 14.04 / Arm

Add unrestricted software available from Universe

    $   sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu $(lsb_release -sc) universe"
    
Update all packages

    $   sudo apt-get update

Install Python and Python-dev

    $   sudo apt-get install python-dev   # install python dev tools
    $   pip install pyephem               # install py ephem

Soft-link hyperterra/ephemScript.py to local python lib location so it's available globally.

    $   ln -s hyperterra/assets/ephemScript.py /usr/local/lib/python2.7/dev-packages/

Clone source and build executable

    $   git clone https://github.com/quilime/hyperterra.git
    $   cd hyperterra
    $   git submodule init --recursive
    $   cd deps/Cinder 
    $   make release # build Cinder "release" Build
    $   cd ../../
    $   make
    
Run

    $   ./linux/bin/release/LandscapeApp
    
Debugging
    
    $   make debug
    $   gdb ./linux/bin/debug/LandscapeApp_D # once in gdb, type 'r' to run

## OSX 10.10

[TDB]
    
    $   ln -s hyperterra/assets/ephemScript.py /Library/Python/2.7/site-packages/


    
## Usage

Controlled by Liine Lemur App
