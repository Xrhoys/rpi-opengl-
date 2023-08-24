@echo off
@set OUT_DIR=debug
@set OUT_EXE=win32_asset
@set INCLUDES=/I ..\include\
@set SOURCES=../asset_build.cpp
@set DEFINES=-DDEBUG=1 -DWIN32=1
@set FLAGS=/nologo /Zi /MT -FC

IF NOT EXIST Debug mkdir debug
pushd %OUT_DIR%

REM Compile asset building code
DEL app*.pdb > NUL 2> NUL

REM Compile platform code
cl %FLAGS% %DEFINES% %INCLUDES% %SOURCES% /Fe%OUT_EXE%.exe /link %LIBS%

popd