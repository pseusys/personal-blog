# Convenience Makefile for ExampleVpnPlugin
#
# This Makefile is intentionally small: it just shortens common Meson/Ninja
# commands for interactive use.

BUILD_DIR ?= build
BUILDTYPE ?= debugoptimized
PREFIX ?= /usr

# If you want to stage installation:
#   make install DESTDIR=$$PWD/_dest
DESTDIR ?=

.PHONY: help setup build install clean reconfigure distclean format tidy compdb docker-build

help:
	@echo "Targets:"
	@echo "  setup        - meson setup $(BUILD_DIR) --buildtype=$(BUILDTYPE)"
	@echo "  build        - meson compile -C $(BUILD_DIR)"
	@echo "  install      - meson install -C $(BUILD_DIR) [DESTDIR=...]"
	@echo "  clean        - meson compile -C $(BUILD_DIR) --clean"
	@echo "  reconfigure  - meson setup --reconfigure $(BUILD_DIR)"
	@echo "  distclean    - rm -rf $(BUILD_DIR) compile_commands.json"
	@echo "  format       - clang-format -i src/*.[ch] plugin/*.[ch]"
	@echo "  compdb       - copy compile_commands.json from build dir"
	@echo "  tidy         - run clang-tidy (uses compile_commands.json)"
	@echo "  docker-build - build repo Containerfile via podman"

setup:
	meson setup $(BUILD_DIR) \
		--buildtype=$(BUILDTYPE) \
		--prefix=$(PREFIX)

reconfigure:
	meson setup --reconfigure $(BUILD_DIR) \
		--buildtype=$(BUILDTYPE) \
		--prefix=$(PREFIX)

build:
	meson compile -C $(BUILD_DIR)

install:
	DESTDIR=$(DESTDIR) meson install -C $(BUILD_DIR)

clean:
	meson compile -C $(BUILD_DIR) --clean

# Format only local sources (does not touch generated files)
format:
	clang-format -i src/*.[ch] plugin/*.[ch]

# Copy the compilation database that Meson already generates in the build dir.
# clang-tidy needs it in the project root.
compdb:
	cp $(BUILD_DIR)/compile_commands.json .

# Run clang-tidy using the compilation database.
# Note: Meson's 'warning_level=everything' emits gcc-specific flags that clang
# does not understand, so we suppress unknown-warning-option errors.
# You can override checks:
#   make tidy TIDY_CHECKS='-*,clang-analyzer-*'
TIDY_CHECKS ?=

SOURCES := src/plugin.c src/editor.c src/interface_gtk3.c src/interface_gtk4.c src/auth_dialog.c

tidy: compdb
	clang-tidy -p . --extra-arg=-Wno-unknown-warning-option $(if $(TIDY_CHECKS),--checks=$(TIDY_CHECKS),) $(SOURCES)

distclean:
	rm -rf $(BUILD_DIR) compile_commands.json

# Builds the top-level repo Containerfile (not inside ExampleVpnPlugin)
# Useful for verifying a clean build and plugin detection.
docker-build:
	cd .. && podman build -f Containerfile -t nm-plugin-buildtest:latest .
