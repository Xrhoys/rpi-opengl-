@echo off
@set OUT_DIR=debug
@set OUT_EXE=win32_main
@set VULKAN_PATH=C:\VulkanSDK\1.3.261.0
@set INCLUDES=/I ..\include\  /I .. /I%VULKAN_PATH%/include
@set SOURCES=../win32_main.cpp
@set LIBS=../lib/libEGL.lib ../lib/libGLESv2.lib ../lib/avcodec.lib ../lib/avformat.lib ../lib/avutil.lib ../lib/avdevice.lib ../lib/avfilter.lib User32.lib ../lib/postproc.lib ../lib/swresample.lib ../lib/swscale.lib %VULKAN_PATH%/lib/vulkan-1.lib
@set DEFINES=-DDEBUG=1 -DBE_VULKAN
@set FLAGS=/nologo /Zi /MT -FC

IF NOT EXIST Debug mkdir debug
pushd %OUT_DIR%

REM Compile app code
DEL app*.pdb > NUL 2> NUL
REM cl %FLAGS% %DEFINES% /LD %INCLUDES% ../app.cpp /DLL /link imgui*.obj -incremental:no -opt:ref -EXPORT:UpdateAndRender -EXPORT:Init

REM Comile platform code
cl %FLAGS% %DEFINES% %INCLUDES% %SOURCES% /Fe%OUT_EXE%.exe /link %LIBS%

popd