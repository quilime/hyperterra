# Hyper Terra

A Landscape that can exist in any time or place on Earth. Light is defined by the Sun and Moon positions of that time and place, calculated by an ephemeris table.

Tested on:
    - Nvidia Jetson TK1
    - Ubuntu 14.04

# Installation

## Ubuntu 14.04

### Python and Python-dev

    $   sudo apt-get install python-dev   # install python dev tools
    $   pip install pyephem               # install py ephem

Soft-link hyperterra/ephemScript.py to local python library.

    $   ln -s hyperterra/assets/ephemScript.py /usr/local/lib/python2.7/dev-packages/

Building

    $   git clone https://github.com/quilime/hyperterra.git
    $   cd hyperterra
    $   git submodule init --recursive
    $   cd deps/Cinder 
    $   make release # build Cinder "release" Build
    $   cd ../../
    $   make
    
Debugging
    
    $   make debug
    $   gdb ./linux/bin/debug/LandscapeApp_D # once in gdb, type 'r' to run
    
Run

    $   ./linux/bin/release/LandscapeApp

## OSX

    $   ln -s hyperterra/assets/ephemScript.py /Library/Python/2.7/site-packages/


    
## Usage

Controlled by Liine Lemur App
    
    
