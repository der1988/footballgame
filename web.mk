CC := emcc
CXX := em++
BUILD_DIR := build/web/objects
RAYLIB_DIR := build/web/raylib/src
SOURCES := $(shell find src -name '*.c' -o -name '*.cpp')
OBJECTS := $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(SOURCES)))
ASSETS := $(shell find res/spritesheets res/animations res/fonts res/shaders -type f) res/pitch.png
FLAGS := -O2 -DPLATFORM_WEB -DDISABLE_TIME_MACHINE -DIsTextureReady=IsTextureValid -I./src -I./src/libs -I./src/libs/imgui -I./src/libs/rlimgui -I$(RAYLIB_DIR) -MMD -MP

.PHONY: all
all: dist/game.js

dist/game.js: $(OBJECTS) $(RAYLIB_DIR)/libraylib.a $(ASSETS) web.mk
	mkdir -p dist
	$(CXX) $(OBJECTS) $(RAYLIB_DIR)/libraylib.a -O2 -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=100663296 -sMAXIMUM_MEMORY=268435456 -sFORCE_FILESYSTEM=1 -sEXIT_RUNTIME=0 --preload-file res/pitch.png@res/pitch.png --preload-file res/spritesheets@res/spritesheets --preload-file res/animations@res/animations --preload-file res/fonts@res/fonts --preload-file res/shaders@res/shaders -o $@

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(FLAGS) -std=c99 -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(FLAGS) -std=c++11 -c $< -o $@

-include $(OBJECTS:.o=.d)
