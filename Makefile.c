# Makefile for The Alan Parsons Project (C version)
# Converted from x86 assembly to C

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

SRC_DIR = source_c
INC_DIR = include_c

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/sdl_wrapper.c $(SRC_DIR)/input.c \
          $(SRC_DIR)/rand.c $(SRC_DIR)/sse_mem.c $(SRC_DIR)/player.c \
          $(SRC_DIR)/mapeng.c $(SRC_DIR)/ppe.c $(SRC_DIR)/enemy.c \
          $(SRC_DIR)/ai.c $(SRC_DIR)/menu.c

HEADERS = $(INC_DIR)/defs.h $(INC_DIR)/graphics.h $(INC_DIR)/input.h \
          $(INC_DIR)/rand.h $(INC_DIR)/sse_mem.h $(INC_DIR)/player.h \
          $(INC_DIR)/mapeng.h $(INC_DIR)/ppe.h $(INC_DIR)/enemy.h \
          $(INC_DIR)/ai.h $(INC_DIR)/menu.h

OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Install dependencies (macOS with Homebrew)
install-deps-macos:
	brew install sdl2 sdl2_mixer

# Install dependencies (Ubuntu/Debian)
install-deps-linux:
	sudo apt-get install libsdl2-dev libsdl2-mixer-dev

.PHONY: all clean run debug install-deps-macos install-deps-linux
