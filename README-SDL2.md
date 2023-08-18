# DtCyber SDL2 Console - Experimental

This is an experimental component to replace the legacy X11 console used for Linux builds. At present, it is minimally functional and rather buggy.


# Notable Working Features
* Dynamic scaling.  Resize the window and contents will scale. 
* Full Screen mode by pressing `F12``.  WARNING - This is VERY buggy.  It may get stuck in full screen mode.
* Uses a TrueType Monospace font instead of bitmapped font.

# Ultimate Goals
* At least feature parity of legacy consoles
* Cross-platform with a single console
* Font will be truetype based for resizing
* Allow changing font, size, and text color without recompiliation
* Maybe autoscaling by just resizing the window.
* Allow full-screen operation, for those that want a dedicated screen for DtCyber. (Completed, but buggy! )


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
sudo apt libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev libsdl2-ttf-2.0-0 libsdl2-ttf-dev
```

### Important - Font Required
You will also need a monospaced truetype font. For initial development I am using [Mozilla's FiraMono-Regular.ttf](https://github.com/mozilla/Fira/blob/master/ttf/FiraMono-Regular.ttf) available on [Mozilla/Fira](https://github.com/mozilla/Fira) under the [OFL license](0https://github.com/mozilla/Fira/blob/master/LICENSE). 

I am not including the font.  You will need to download it and copy the font into the directory where you launch `dtcyber` (or `node start`).
