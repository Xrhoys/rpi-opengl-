
export OUTPUT_DIR="debug"
export OUTPUT_EXE="linux-main"

export CXX_FLAGS="-O2 -ggdb -Wno-write-strings -Wno-shift-count-overflow -DDEBUG"
export LIBS="-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lEGL -lGLESv2 -lpostproc -lswresample -lswscale -lstdc++ -lX11 -lm -lva"

rm $OUTPUT_DIR/$OUTPUT_EXE
g++ $CXX_FLAGS linux_main.cpp -o $OUTPUT_DIR/$OUTPUT_EXE $LIBS
chmod +x $OUTPUT_DIR/$OUTPUT_EXE