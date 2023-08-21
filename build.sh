
export OUTPUT_DIR="debug"
export OUTPUT_EXE="linux-main"

export CXX_FLAGS="-O2 -ggdb -Wno-write-strings"
export LIBRARIES="-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lEGL -lGLESv2 -lpostproc -lswresample -lswscale -lstdc++ -lX11 -lm -lvulkan"

export FLAGS="-DDEBUG -DBE_VULKAN"

rm $OUTPUT_DIR/$OUTPUT_EXE
g++ $FLAGS $CXX_FLAGS $LIBRARIES linux_main.cpp -o $OUTPUT_DIR/$OUTPUT_EXE
chmod +x $OUTPUT_DIR/$OUTPUT_EXE