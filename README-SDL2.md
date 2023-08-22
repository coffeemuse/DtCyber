# DtCyber SDL2 Console - Experimental

This is an experimental component to replace the legacy X11 console used for Linux builds. At present, it is minimally functional and rather buggy.

# Notable Working Features
* Dynamic scaling.  Resize the window and contents will scale. 

# Ultimate Goals
* At least feature parity of legacy consoles
* Cross-platform with a single console


# Roadmap - roughly in chronological order
* Feature parity with existing X11 console for Linux x86_64 builds only.
* Allow specifying font size at launch
* Autoscaling by window resizing
* Add Linux ARM64 support. 
* Add macOS M1/M2 support.
* Add macOS x86_64 bit support.
* Add Windows support
* Add 32-bit support, if there is interest.  However, most modern operating systems have been 64 bit for several years. 32-bit support will be low priority.


# Build Notes
My current working platform is Ubuntu Desktop 22.04.3 LTS, so you may need to adapt these instructions for your environment.

## Prerequisites
Follow the guidelines already document for DtCyber.  However, you will need to install some SDL2 related packages.

```bash
sudo apt libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev 
```