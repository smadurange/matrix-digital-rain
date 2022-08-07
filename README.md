# Matrix Digital Rain

![example](example.png)

> Do not try and bend the spoon, that's impossble. Instead, only try to realize the truth. There is no spoon.

 -- _Spoon Boy_

## Overview

Pure C implementation of the famous [digital rain](https://en.wikipedia.org/wiki/Matrix_digital_rain) effect from _The Matrix_ series for Linux inspired by [fakesteak](https://github.com/domsson/fakesteak). While trying to keep the simplicity and lightweightness of fakesteak as much as possible, I have added the following characteristics.

 - Simulation pattern is closer to the one seen in the background while Neo and Cypher were talking in the first Matrix movie.
 - Ghosting effect of monochrome displays.
 - Truecolor support and simple colour customisation options to match the theme of the setup for ricing. 
 - Unicode support.

## Requirements and Dependencies

 - Terminal emulator with support for 24-bit RGB colours and unicode characters.

## Customisation

 - Character set: set `UNICODE_MIN` and `UNICODE_MAX` for the [unicode block](https://en.wikipedia.org/wiki/List_of_Unicode_characters) you like to use (e.g. 0x30A1 and 0x30F6 for Katakana).
 - Colours: set the RGB values of `COLOR_BG_*`, `COLOR_HD_*` and `COLOR_TL_*` for background, head and the tail characters respectively.
 - Rain attributes: set `RAIN_RATE` and `RAIN_DENSITY` to change the speed and the density of the rain. 

## Building and Running

All the code is in a single file (src/main.c) and has no external dependencies. You can compile it any way you like. If you have autotools installed you can build and run using the following commands.
```
autoreconf -i
cd build/
../configure
make
./amx
```

## Contributing

Code is hosted on [sourcehut](https://git.sr.ht/~sadeep/matrix-digital-rain). Please submit your patches there or via email to sadeep@asciimx.com ([PGP key](http://www.asciimx.com/sadeep.asc)).
