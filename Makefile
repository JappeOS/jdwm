# Makefile for JDWM Flutter backend native library

NATIVE_DIR := native

# Pass through build metadata if provided
BUILD_VERSION ?= local
BUILD_GIT_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo nogit)
BUILD_TIMESTAMP ?= $(shell date -u +%Y-%m-%dT%H:%M:%SZ)

WLR_INC_SUBDIR ?= wlroots-0.19

NATIVE_MAKE_ARGS = \
	WLR_ROOT=$(abspath $(WLR_ROOT)) \
	WLR_SRC_ROOT=$(abspath $(WLR_SRC_ROOT)) \
	WLR_INC_SUBDIR=$(WLR_INC_SUBDIR) \
	BUILD_VERSION=$(BUILD_VERSION) \
	BUILD_GIT_COMMIT=$(BUILD_GIT_COMMIT) \
	BUILD_TIMESTAMP=$(BUILD_TIMESTAMP)

.PHONY: debug profile release clean

all: debug

debug:
	$(MAKE) -C $(NATIVE_DIR) debug $(NATIVE_MAKE_ARGS)

profile:
	$(MAKE) -C $(NATIVE_DIR) profile $(NATIVE_MAKE_ARGS)

release:
	$(MAKE) -C $(NATIVE_DIR) release $(NATIVE_MAKE_ARGS)

clean:
	$(MAKE) -C $(NATIVE_DIR) clean
