EXECUTABLE=linux-asset
TARGET=linux-asset
BIN=debug
LIBS="-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lEGL -lGLESv2 -lpostproc -lswresample -lswscale -lstdc++ -lX11 -lm"
FLAGS="-O2 -ggdb -Wno-write-strings -D LINUX"

gcc $FLAGS $LIBS asset_build.cpp -o $BIN/$TARGET 