# n64texturefiltering

This was an experiment to reproduce the "unique" texture filtering of the Nintendo 64.

The N64 was the first 3D home video game console to have texture filtering of any kind. It supported bilinear filtering, but unlike virtually every bilinear texture filtering implementation since, it used only 3 samples instead of 4. The result being that textures would appear "smeared" along one diagonal direction.

See: http://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro14/14-01.html

Some games used this to their advantage, so some textures under N64 emulators, including Nintendo's own, can look a lot worse than they did on original hardware.

I wanted to understand and reproduce how the N64 hardware implemented texture filtering.

Written in C and using SDL, this program will display any image at any size applying the 3-point texture sampling. I don't know if it is correct, but it does reproduce the right look.

Obviously in the real world you would probably write this as a GPU pixel shader, but it was fun to reproduce a pixel pipeline on the CPU.

Theres also some experimentation with SIMD types and intrinsics.

Currently Xcode only, would like to move to a Makefile.
