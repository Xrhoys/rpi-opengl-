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

# Pres

## Original goals and constraints

It aims to run videos on a Raspberry Pi, and eventually UI elements and render at an acceptable frame rate.
4k30 / 1080p60
Do not burn down the hardware, assure longevity at reasonable cooling/noise (45db?)
No installation needed, a single executable does the trick.

This will lead to several hard constraints:
- The software that runs on the RPI has to be performant at all cost
- Can not to be written in anything else than a compiled language
- Do not depend on any API/Framework/Library for total control over performance

Nice to have:
- Portability
- Simplicity 
- Hot reloading

# Current Architecture

First, the code is written and compiled in 
- Windows: MSVC (Microsoft Visual Compiler)
- GCC/G++ on unix platforms

The build is done in what's called in "Unity build" which means there's one entry point for each runtime, so it does not do incremental compilation at this stage of the project

2 entry points exists for each platform, win32_main.cpp and linux_main.cpp.
The platform (OS) is abstracted as a layer (platform.h) and the app itself simply calls platform dependent interface. For instance reading a file on disk is to simply retrieve the appContext where the `DebugReadEntireFile` is.
I will spare the details, but it's basically a combination of typedef stubs and function pointers.
Here is a small list of what is in it: device event, graphics api init, memory init, os app/window init, timer api, ...

At its root, the main() function of those two entry points does the following
```
main()
{
	Initialize the app, all platform dependent calls 

	for(;;) // App loop
	{
		// Process input
		// Process OS event
		// Update app states
		// Render()
	}
	
	// Clean up app remaining app states, end renderer instance, window instance
}
```

Especially the App part includes two functions: InitApp() and UpdateAndRenderApp(). The former is called at the end of the initialization phase, the second one one is called every frame.

Render() is where the actual app is shown to the user. So in a nutshell, it all goes in this order: read event/input, update app states, render.

As you can see, the APP part of the code is completely separated and only interacts with runtime interface. Which means this app can and will be able to be ported to any platform without too much work, that's what i mean by __portability__ and not __cross-platform__ to avoid that extra layer of friction and uncertainty.


## Video decoding

FFMPEG: actually ffmpeg is the executable, the actual library is libavcodec. It is still experimental right now, so i'm basically decoding every frame which, currently I have no idea how much time it takes per frame. So the performance impact of this lib is still unknown. However, this can be cached and pre-rasterized as asset during loading instead of being decoded each frame.
That said, a lower hanging fruit is available, which is to use hardware acceleration especially on RPI: Intel DRM. 

## Rendering routine (renderer.cpp)
The project uses OpenGL ES, which a reduced set of functionalities compared to the normal OpenGL api. It is used in embedded devices such as Rpi and Smartphones.
For the development, an simulation layer EGL + GLES is used, so the same API interface is used. 

### Rendering pipeline
Shader code, GPU interaction 
A bit of rendering basics: vertex shader, fragment shader, among others
[Explain roughly what the gpu does to print something on the screen: rasterization]

### Actually set
Vertex shader: goes through all vertices and transforms data on the GPU
Fragment shader: goes through each pixel and apply shader code

Practical: vertex shader to transform to homogenous v4 coordinate system in clip space [-1, 1], then in fragment shader, query texture coordinates and apply color to the pixel.

# API and library (or lack of)

Common Runtime: libavcodec, stdint, EGL, GLES
Common builder: stb_h, stdio.h 

(mostly OS)
Windows: Win32, 
Linux: X11, Posix API

=> We try to minimize the use of API so we don't get dependency hell + easy portability, and get to actually control the performance, which is critical in this project.

# Prototyping before the end of the year

The current project has the following features:
- 2d rendering pipeline (background video, font, UI)
- font engine rendering
- video decoding routine
- UI structure

About the UI structure, it will be architected in the following way

Using the Immediate Mode GUI pattern, the two part of the UI will be produced:

- CORE UI API: routines to produce the data being processed at the rendering level (position, width)
- BUILDER UI API: collection of helper functions to compose more complex UI

# Projection, Shippable Code, Performance

Video is just a small part of the plan, it was an toy project to kinda prove if an RPI can withstand the load with just libavcodec.

So here is what I'm projecting:
- get rid of libavcodec to have a fully custom video rendering pipeline with minimal CPU overhead
- build an UI API that is cachable (on disk and in memory): the purpose of this is to be able to just dump it in a custom format file and load it on the fly. Why is this important? First, I wanna build hot-reload feature to ease development process and also use it for the desktop editor client. Second, this UI will become actually both the client facing editor and also as debug mode console.
- integrate perfomance measuring profilers or make my own, cause why not.
- if the experiment of fully custom video rendering pipeline is a success, then proceed to optimize improve it until it renders 30fps in 4k, at least (there are few ways i'm thinking about currently to attain that, one of them is getting rid of X11)

The code is shippeable when:
- API platform layers should integrate error handling and graceful termination (caching)
- ??

# Vision and caveats

The vision is to build an entire suite of tools that helps the user to design and interact with their device.
That includes a live editing tool that runs on Web (WebGL), Windows, Linux, Mac, which either upload assets, or enters live mode, to edit while connected the real device.

Bonus: create C bindings to different languages, so you can write in any popular language and have it directly produce UI assets.

The problem remains that this UI asset might be very big to store. The biggest problem being video data

For that, there are few pieces that needs to be solved, all based on the fundamental promise that the USER EXPERIENCE is king. We could surely produce something that works but is slow and buggy. But that's not fun and will not sell.

THOUGH!!!
I have made several toy 3d engine before, but that's my first time making production ready UI, so here is the caveat, there's no guarantee that I will succeed to make everything happen, especially not full time on this. But I do have the required knowledge and experience to push pretty far, and I know what good software look like. Now the other concern is, even if I was able to produce all the promises, who would be working on that along? Everything would be custom all the way till memory allocation and probably incorporate some hardcore optimization at assembly level.

# Business model

Device bundling is one way to do it, but you can also sell software support and upgrades