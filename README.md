# RPI Experimentatal media player + UI engine

## Configure

Create a data folder and place in the font (arial.ttf) and the mp4 file (sample.mp4).

## Compiling

### Linux

Run this script to configure the asset data & compile the program.

```
./build_linux.sh
```

Alternatively, you can run `build.sh` to compile the main program or `build_asset.sh` to build the asset bundle.

### Windows

```
build.bat
```

This project uses EGL and GLES (even on desktop) for its graphics API.
The shippable version is and will be written from scratch.

The asset processor and toolings can use library/frameworks.

## To RUN

Run the asset builder executable to produce an `asset_data` file and put that in a `data` folder at the same level than current work directory.
Also add an `sample.mp4` video file.

In order to run the asset_build executable currently, it needs `ariat.ttf` file that you can download easily from internet.
Or if you work on Windows: `C:/Windows/Fonts/arial.ttf`

```
debug
|--data
|  |
|  |--- asset_data
|  |--- sample.mp4
|  |--- arial.ttf
|
|  linux-main (or win32_main)
```

## Development

Currently the project uses 4Coder as its main editor and Remedybg for the debugger.
RenderDoc does not work correctly with EGL, but eventually would be a nice addition.

On linux, there's no debugger that's actually useful. You can try gdb bundled with vscode, but that's about it.
If someone knows more about this, he or she is welcomed to complete this process.

## TODOs

- ~~Start building a light asset processor pipeline (in progress...)~~
- ~~Using stb_truetype library to load font and render pre-rasterized bitmaps.~~
  ~~This is only for debugging purpose right now, in the future the font pipeline should be able to support vectorized font data.~~
- ~~Handle keyboard/mouse/os events (portable way, in platform.h)~~
- Networking basics, connect and design an additiona UDP client that can stream data to the app
- Move the renderer and decoder each in its separate thread

## Exploring

- Explore UI structures in immediate mode (in progress...)
