ARCH := $(firstword $(subst -, ,$(shell $(CC) -dumpmachine)))
ifeq ($(ARCH),x86_64)
else ifeq ($(ARCH),aarch64)
else ifneq ($(findstring $(ARCH),arm),)
ARCH := armhf
else
ARCH := x86
endif

# Program
EXECUTABLE	:= linux-main

# Compiler and Linker
CC	:= gcc
LD	:= gcc

# The Target Binary Program
TARGET	:= linux-main

# The Directories, Source, Includes, Objects, Binary and Resources
MODULES	:= EGL GLES GLES2 GLES3 KHR libavcodec libavdevice libavfilter libavformat libavutil libpostproc libswresample libswscale
BIN		:= debug
SRC_DIR		:= src
INCLUDE_DIR		:= $(addprefix include/,$(MODULES))
BUILD_DIR		:= $(addprefix build/,$(MODULES))
LIB_DIR		:= $(addprefix lib/,$(MODULES))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ		:= $(patsubst ./%.cpp,build/%.o,$(SRC))
INCLUDE		:= $(addprefix -I , $(INCLUDE_DIR))
LIB		::= $(addprefix -L , $(LIB_DIR))

# Flags, Libraries and Includes
CXX		  := gcc
CXX_FLAGS := -O2 -ggdb -Wno-write-strings
LIBRARIES := -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lGL -lGLU -lEGL -lGLESv2 -lpostproc -lswresample -lswscale -lstdc++ -lX11
vpath %.cpp $(SRC_DIR)

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
# Default Make
all: $(BIN)/$(EXECUTABLE)

dev: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

run:
	clear 
	./$(BIN)/$(EXECUTABLE)

# For issues with vim not recognizing header files
bear: clean all
	echo "Generated compile_commands.json"

$(BIN)/$(EXECUTABLE): linux_main.cpp
	mkdir -p ${BIN}
	$(CXX) $(CXX_FLAGS) $^ $(LIBRARIES) -o $@
	chmod +x $(BIN)/${EXECUTABLE}

# $(BIN)/$(EXECUTABLE): linux_debug.cpp
# 	mkdir -p ${BIN}
# 	$(CXX) -O0 -ggdb -I include $(INCLUDE) $^ $(LIBRARIES) -o $@
# 	chmod +x $(BIN)/${EXECUTABLE}

install:
	@echo "** Installing..."
	sudo cp $(BIN)/$(EXECUTABLE) /usr/bin

uninstall:
	@echo "** Uninstalling..."
	sudo rm -f /usr/bin/$(EXECUTABLE)

clean:
	-rm $(BIN)/*

