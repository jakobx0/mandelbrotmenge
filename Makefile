BUILD_DIR ?= build/debug
CMAKE ?= cmake
WIDTH ?= 1280
HEIGHT ?= 720
PORT ?= 5000
HOST ?= 127.0.0.1

.PHONY: all deps-arch configure build run-server run-client clean

all: build

deps-arch:
	sudo pacman -S --needed base-devel cmake boost sdl12-compat

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMKR_SKIP_GENERATION=ON
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

run-server: build
	./$(BUILD_DIR)/server $(WIDTH) $(HEIGHT) $(PORT)

run-client: build
	./$(BUILD_DIR)/client $(HOST) $(PORT)

clean:
	rm -rf build compile_commands.json
