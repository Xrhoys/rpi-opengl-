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

## TODOs

- ~~Start building a light asset processor pipeline (in progress...)~~
- ~~Using stb_truetype library to load font and render pre-rasterized bitmaps.
  This is only for debugging purpose right now, in the future the font pipeline should be able to support vectorized font data.~~
- ~~Handle keyboard/mouse/os events (portable way, in platform.h)~~
- Networking basics, connect and design an additiona UDP client that can stream data to the app
- Move the renderer and decoder each in its separate thread

## Exploring

- Explore UI structures in immediate mode (in progress...)

## Vulkan decoder backend

There are some issues related to decoding the frames with either FFMPEG or DRM indirectly as backend.
Both of them will need to send the data to the GPU buffer, being decoded, then sent back to system memory.
From here, being uploaded back to the GPU again as texture, eventually one more routine at fragment shader level to convert 
for instance NV12 - 4:2:0 YUV planar pixel format to float RGB, which is the shader return type.

This wastes a lot of system resources due to the double round trip needed (very slow), plus pixel conversions all over the place.
It is also an issue from using  different driver implementation, for instance on RPI, the Intel DRM hardware API (hardware decoding of H265), will produce its own buffer type, which can not be directly used by OpenGL. We could use OpenCL, but it does not have video encoders implemented.
 
Rather, since RPI supports Vulkan (v1.2, which I just checked supports video encoders especially H264 and H265), we could simply use it both as the rendering backend, and the decoder backend.
Doing that could potentially reduce CPU overhead.
The only problem is the learning curve of Vulkan is quite steep, and the API has the reputation of being very verbose.
