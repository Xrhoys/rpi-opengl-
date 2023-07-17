# RPI Experimentatal media player + UI engine

For linux version:

```
make clean && make
```

For windows version:

```
build.bat
```

This project uses EGL and GLES (even on desktop) for its graphics API.
The shippable version is and will be written from scratch.

The asset processor and toolings can use library/frameworks.

## TODOs

- Start building a light asset processor pipeline (in progress...)
- Using stb_truetype library to load font and render pre-rasterized bitmaps.
  This is only for debugging purpose right now, in the future the font pipeline should be able to support vectorized font data.
- Handle keyboard/mouse/os events (portable way, in platform.h)
- Networking basics, connect and design an additiona UDP client that can stream data to the app
- Move the renderer and decoder each in its separate thread

## Exploring

- Explore UI structures in immediate mode
