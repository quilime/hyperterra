Tested on an Nvidia TK1 running Ubuntu 14.04

## Install Python and Python-dev

    $   sudo apt-get install python-dev   # install python dev tools
    $   pip install pyephem               # install py ephem

Soft-link the landscape-cinder/ephemScript.py to local python lib

  - osx `/Library/Python/2.7/site-packages/`
  - linux `/usr/local/lib/python2.7/dev-packages/`

## Building

    $   git clone https://github.com/quilime/hyperterra.git
    $   cd hyperterra
    $   git submodule init --recursive
    $   cd deps/Cinder 
    $   make release # build Cinder "release" Build
    $   cd ../../
    $   make
    
## Debugging
    
    $   make debug
    $   gdb ./linux/bin/debug/LandscapeApp_D # once in gdb, type 'r' to run
    
## Running

    $   ./linux/bin/release/LandscapeApp
    
## Usage

Controlled by Liine Lemur App
    
    
