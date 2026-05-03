CC := clang
CXX := clang

# Main executable
TARGET_EXEC := thegame

BUILD_DIR := ./build
SRC_DIRS := ./src

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lstdc++

CFLAGS := -ggdb -std=c99 -Wall -Wextra -O0

# Optional video export support (requires ffmpeg-dev)
ENABLE_VIDEO_EXPORT ?= 1
ifeq ($(ENABLE_VIDEO_EXPORT),1)
  CFLAGS += -DHAS_FFMPEG
  LDFLAGS += -lavformat -lavcodec -lswscale -lswresample -lavutil
endif

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Tests
TEST_SRC_DIRS := ./test
TEST_BUILD_DIR := $(BUILD_DIR)/test

UNITY_SRC_DIRS := $(TEST_SRC_DIRS)/unity

TESTS := $(shell find $(TEST_SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
TEST_OBJS := $(TESTS:%=$(TEST_BUILD_DIR)/%.o)

TEST_FLAGS := -DRUNNING_TEST

# Source objects recompiled for tests (with TEST_FLAGS)
TEST_SRC_OBJS := $(SRCS:%=$(TEST_BUILD_DIR)/%.o)

# Build step for C source (test build)
$(TEST_BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

# Build step for C++ source (test build)
$(TEST_BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_FLAGS) -c $< -o $@

.PHONY: test
test: $(BUILD_DIR)/test_caches $(BUILD_DIR)/test_memory $(BUILD_DIR)/test_actions $(BUILD_DIR)/test_entity_repo $(BUILD_DIR)/test_events
	@failed=""; \
	for t in test_caches test_memory test_actions test_entity_repo test_events; do \
		echo; echo "=== Testing $$t ==="; echo; \
		$(BUILD_DIR)/$$t || failed="$$failed $$t"; \
	done; \
	echo; \
	if [ -n "$$failed" ]; then \
		echo "FAILED suites:$$failed"; exit 1; \
	else \
		echo "All test suites passed."; \
	fi

test_caches: $(BUILD_DIR)/test_caches
	@echo
	@echo === Testing caches ===
	@echo
	$(BUILD_DIR)/$@

test_memory: $(BUILD_DIR)/test_memory
	@echo
	@echo === Testing memory ===
	@echo
	$(BUILD_DIR)/$@

test_actions: $(BUILD_DIR)/test_actions
	@echo
	@echo === Testing actions ===
	@echo
	$(BUILD_DIR)/$@

test_entity_repo: $(BUILD_DIR)/test_entity_repo
	@echo
	@echo === Testing entity repo ===
	@echo
	$(BUILD_DIR)/$@

test_events: $(BUILD_DIR)/test_events
	@echo
	@echo === Testing events ===
	@echo
	$(BUILD_DIR)/$@

$(BUILD_DIR)/test_caches: $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CXX) $(filter $(TEST_BUILD_DIR)/$(TEST_SRC_DIRS)/$(notdir $@).c.o $(TEST_BUILD_DIR)/$(UNITY_SRC_DIRS)/%.c.o, $(TEST_OBJS)) $(filter-out $(TEST_BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(TEST_SRC_OBJS)) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_memory: $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CXX) $(filter $(TEST_BUILD_DIR)/$(TEST_SRC_DIRS)/$(notdir $@).c.o $(TEST_BUILD_DIR)/$(UNITY_SRC_DIRS)/%.c.o, $(TEST_OBJS)) $(filter-out $(TEST_BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(TEST_SRC_OBJS)) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_actions: $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CXX) $(filter $(TEST_BUILD_DIR)/$(TEST_SRC_DIRS)/$(notdir $@).c.o $(TEST_BUILD_DIR)/$(UNITY_SRC_DIRS)/%.c.o, $(TEST_OBJS)) $(filter-out $(TEST_BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(TEST_SRC_OBJS)) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_entity_repo: $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CXX) $(filter $(TEST_BUILD_DIR)/$(TEST_SRC_DIRS)/$(notdir $@).c.o $(TEST_BUILD_DIR)/$(UNITY_SRC_DIRS)/%.c.o, $(TEST_OBJS)) $(filter-out $(TEST_BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(TEST_SRC_OBJS)) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_events: $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CXX) $(filter $(TEST_BUILD_DIR)/$(TEST_SRC_DIRS)/$(notdir $@).c.o $(TEST_BUILD_DIR)/$(UNITY_SRC_DIRS)/%.c.o, $(TEST_OBJS)) $(filter-out $(TEST_BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(TEST_SRC_OBJS)) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
