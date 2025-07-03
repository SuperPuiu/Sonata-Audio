# Sonata Audio
Sonata Audiois simply a program which can be used to play all sorts of audio files, as long as they're supported by SDL_mixer. The project started as a way to improve my C programming skill and also as a way to learn how to use microui. A lot of the design choices were made with the idea of using malloc() as little as possible. It should be also noted that the program attemps to use as little resources as it can, which is why it handles drawing itself instead of letting SDL do it.

# Building
**Linux:**
You must have installed SDL3 and SDL3_mixer. The header files are included from `include/SDL3/` and `include/SDL3_mixer/` respectively.
Use `make linux` to build a release version. Use `make` to build a linux test version of the program.

**Windows:**
You must have installed SDL3 and SDL3_mixer. The header files are included from `include/SDL3/`. You must also have the mingw compiler installed. Use `make windows` to build the windows version of the program.

**MacOS:**
Not supported.

# License
The project is licensed under GPL-3.0.
