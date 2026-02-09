# Makefile for The Alan Parsons Project (C version)
# Originally x86 assembly (2002), now clean C architecture
#
# Targets:
#   all       - Build the pure architecture (default)
#   legacy    - Build the legacy architecture (old main.c)
#   clean     - Remove build artifacts
#   run       - Build and run
#   test      - Run test suite

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -Iinclude_c -I/usr/local/include/SDL2
LDFLAGS = -L/usr/local/lib
LIBS = -lSDL2 -lSDL2_mixer -lm

# On macOS with Homebrew:
ifeq ($(shell uname), Darwin)
    CFLAGS += -I/opt/homebrew/include/SDL2
    LDFLAGS += -L/opt/homebrew/lib
endif

# On Linux:
# CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_mixer)
# LIBS = $(shell pkg-config --libs sdl2 SDL2_mixer) -lm

TARGET = alan_parsons_c
TARGET_PURE = alan_parsons_pure

SRC_DIR = source_c
INC_DIR = include_c

# Core sources (shared between architectures)
CORE_SOURCES = $(SRC_DIR)/sdl_wrapper.c $(SRC_DIR)/input.c \
               $(SRC_DIR)/rand.c $(SRC_DIR)/sse_mem.c \
               $(SRC_DIR)/mapeng.c $(SRC_DIR)/ppe.c \
               $(SRC_DIR)/menu.c $(SRC_DIR)/game/menu_new.c \
               $(SRC_DIR)/game/game.c $(SRC_DIR)/game/weapons.c \
               $(SRC_DIR)/game/render.c $(SRC_DIR)/game/sprites.c

# Legacy sources (only for legacy build)
LEGACY_SOURCES = $(SRC_DIR)/player.c $(SRC_DIR)/enemy.c $(SRC_DIR)/ai.c \
                 $(SRC_DIR)/game/bridge.c $(SRC_DIR)/game/audio_bridge.c

# Stub sources (minimal compatibility stubs for pure build)
STUB_SOURCES = $(SRC_DIR)/player.c $(SRC_DIR)/enemy.c $(SRC_DIR)/ai.c \
               $(SRC_DIR)/game/audio_bridge.c

# New platform sources
PLATFORM_SOURCES = $(SRC_DIR)/platform/app.c $(SRC_DIR)/platform/sdl_platform.c

# Legacy build (old main.c with bridge)
SOURCES = $(SRC_DIR)/main.c $(CORE_SOURCES) $(LEGACY_SOURCES)
OBJECTS = $(SOURCES:.c=.o)

# Pure build (new architecture - no bridge)
PURE_SOURCES = $(SRC_DIR)/main_pure.c $(CORE_SOURCES) $(STUB_SOURCES) $(PLATFORM_SOURCES)
PURE_OBJECTS = $(PURE_SOURCES:.c=.o)

HEADERS = $(INC_DIR)/defs.h $(INC_DIR)/input.h \
          $(INC_DIR)/rand.h $(INC_DIR)/sse_mem.h $(INC_DIR)/player.h \
          $(INC_DIR)/mapeng.h $(INC_DIR)/ppe.h $(INC_DIR)/enemy.h \
          $(INC_DIR)/ai.h $(INC_DIR)/menu.h \
          $(INC_DIR)/game/game.h $(INC_DIR)/core/types.h \
          $(INC_DIR)/core/constants.h $(INC_DIR)/math/vec2.h \
          $(INC_DIR)/platform/app.h $(INC_DIR)/platform/platform.h

# Default target - pure architecture (the clean path)
all: $(TARGET_PURE)

# Legacy architecture build (old main.c with bridge)
legacy: $(TARGET)

# Link the legacy executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Link the pure executable
$(TARGET_PURE): $(PURE_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(PURE_OBJECTS) $(TARGET) $(TARGET_PURE)
	find $(SRC_DIR) -name "*.o" -delete

# Run the program (pure architecture)
run: $(TARGET_PURE)
	./$(TARGET_PURE)

# Run legacy version
run-legacy: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Test build (uses test harness)
test:
	$(MAKE) -C tests

# Install dependencies (macOS with Homebrew)
install-deps-macos:
	brew install sdl2 sdl2_mixer

# Install dependencies (Ubuntu/Debian)
install-deps-linux:
	sudo apt-get install libsdl2-dev libsdl2-mixer-dev

.PHONY: all legacy clean run run-legacy debug test install-deps-macos install-deps-linux
