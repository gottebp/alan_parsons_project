# Test Suite Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I../include_c -g

# Game source files needed by tests
GAME_SRC = ../source_c/game/game.c ../source_c/game/weapons.c stubs/menu_stub.c
AI_SRC = ../source_c/game/ai_player.c
BRIDGE_SRC = ../source_c/game/bridge.c stubs/audio_bridge_stub.c

# Test executables
TESTS = test_vec2 test_pool test_game test_playthrough test_ai_selfplay test_bridge test_bridge_opaque test_render test_audio

# Build all tests
all: $(TESTS)
	@echo "All tests built successfully"

# Run all tests
test: $(TESTS)
	@echo ""
	@echo "Running test suite..."
	@echo "======================================"
	@./test_vec2 || true
	@./test_pool || true
	@./test_game || true
	@./test_playthrough || true
	@./test_ai_selfplay || true
	@./test_bridge || true
	@./test_bridge_opaque || true
	@./test_render || true
	@./test_audio || true
	@echo ""
	@echo "Test suite complete"

# Quick test (skips extended tests)
quick: test_vec2 test_pool test_game
	@echo ""
	@echo "Running quick tests..."
	@./test_vec2 || true
	@./test_pool || true
	@./test_game || true

# Individual test executables
test_vec2: test_vec2.c test_framework.h ../include_c/math/vec2.h
	$(CC) $(CFLAGS) -o $@ test_vec2.c -lm

test_pool: test_pool.c test_framework.h ../include_c/core/pool.h
	$(CC) $(CFLAGS) -o $@ test_pool.c

test_game: test_game.c test_framework.h ../include_c/core/types.h ../include_c/game/game.h $(GAME_SRC)
	$(CC) $(CFLAGS) -o $@ test_game.c $(GAME_SRC) -lm

test_playthrough: test_playthrough.c test_framework.h ../include_c/game/game.h $(GAME_SRC)
	$(CC) $(CFLAGS) -o $@ test_playthrough.c $(GAME_SRC) -lm

test_ai_selfplay: test_ai_selfplay.c test_framework.h ../include_c/game/game.h ../include_c/game/ai_player.h $(GAME_SRC) $(AI_SRC)
	$(CC) $(CFLAGS) -o $@ test_ai_selfplay.c $(GAME_SRC) $(AI_SRC) -lm

test_bridge: test_bridge.c ../include_c/game/game.h ../include_c/game/bridge.h $(GAME_SRC) $(BRIDGE_SRC)
	$(CC) $(CFLAGS) -o $@ test_bridge.c $(GAME_SRC) $(BRIDGE_SRC) -lm

test_bridge_opaque: test_bridge_opaque.c ../include_c/game/bridge_opaque.h ../include_c/defs.h $(GAME_SRC) $(BRIDGE_SRC)
	$(CC) $(CFLAGS) -o $@ test_bridge_opaque.c $(GAME_SRC) $(BRIDGE_SRC) -lm

test_render: test_render.c test_framework.h ../include_c/game/game.h ../include_c/core/constants.h
	$(CC) $(CFLAGS) -o $@ test_render.c -lm

test_audio: test_audio.c test_framework.h ../include_c/game/audio.h
	$(CC) $(CFLAGS) -o $@ test_audio.c

# Clean
clean:
	rm -f $(TESTS)

.PHONY: all test quick clean
