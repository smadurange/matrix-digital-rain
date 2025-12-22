MATRIX DIGITAL RAIN

Pure C implementation of the famous digital rain effect from The Matrix,
inspired [fakesteak](https://github.com/domsson/fakesteak). While trying to
keep the simplicity and lightweightness of fakesteak as much as possible, I
have added the following characteristics.

 - Simulation pattern is closer to the one seen during Neo and Cypher's
   conversation in the first Matrix movie.
 - Ghosting effect of monochrome CRT displays.
 - Truecolor support.
 - Unicode support.

REQUIREMENTS

 - Terminal emulator with support for 24-bit RGB colours and unicode
   characters.

CUSTOMISATION

 - Character set: add [unicode
   blocks](https://en.wikipedia.org/wiki/List_of_Unicode_characters) (Katakana: font ja-sazanami-ttf).
   you'd like to use to glyphs. ASCII and Katakana are included by default.
 - Colours: set the RGB values of `COLOR_BG_*`, `COLOR_HD_*` and `COLOR_TL_*`
   for background, head and the tail characters respectively.
 - Rain attributes: set `RHO` to change the density of the rain. 

BUILDING AND RUNNING

With Katakana:

 $ cc -O3 main.c -o matrix
 $ ./matrix

Without Katakana:

 $ cc -O3 -DNOKANA main.c -o matrix
 $ ./matrix

