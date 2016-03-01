# midi_cv_ctrl

This library is an OpenCV interface to MIDI physical controller.

## Description

midi_cv_ctrl is an Windows library is an OpenCV interface to MIDI physical controller using Windows Multimedia Library(winmm).
User can control OpenCV parameters via MIDI physical controller.

## Features

- You can get the values of sliders, volumes and buttons of your MIDI physical controller via MIDI i/f in polling style.

## Requirement

- OpenCV 3.0.0 or above is preferable.
- Checked with win7 32bit + msys2 + gcc (with c++11 support)
- Checked with EDIROL UR-80 (very old one) + UM-1G MIDI USB interface.
	
## Installation

1. In your source file, just include midi_cv_ctrl.h.
2. As for Makefile, refer to attached one, and modify it or create new one according to your host program.
3. Do not forget to link winmm.lib (libwinmm)
4. make

## API Usage

- Please refer to midi_cv_ctrl.h with some API usage comments and sample main.cpp to see how to call APIs.

## Author

delphinus1024

## License

[MIT](https://raw.githubusercontent.com/delphinus1024/midi_cv_ctrl/master/LICENSE.txt)

