# Matrix Digital Rain

Pure C implementation of the famous [digital
rain](https://en.wikipedia.org/wiki/Matrix_digital_rain) effect from _The
Matrix_ series for Linux inspired by
[fakesteak](https://github.com/domsson/fakesteak). While trying to keep the
simplicity and lightweightness of fakesteak as much as possible, I have added
the following characteristics.

 - Simulation pattern is closer to the one seen during Neo and Cypher's
   conversation in the first Matrix movie.
 - Ghosting effect of monochrome CRT displays.
 - Truecolor support.
 - Unicode support.

## Requirements and Dependencies

 - Terminal emulator with support for 24-bit RGB colours and unicode
   characters.

## Customisation

 - Character set: set `UNICODE_MIN` and `UNICODE_MAX` for the [unicode
   block](https://en.wikipedia.org/wiki/List_of_Unicode_characters) you like to
   use (e.g. 0x30A1 and 0x30F6 for Katakana: font ja-sazanami-ttf).
 - Colours: set the RGB values of `COLOR_BG_*`, `COLOR_HD_*` and `COLOR_TL_*`
   for background, head and the tail characters respectively.
 - Rain attributes: set `RAIN_RATE` and `RAIN_DENSITY` to change the speed and
   the density of the rain. 

## Building and Running

 $ cc -O3 main.c -o matrix
 $ ./matrix
