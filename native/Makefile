# Makefile for Zenith backend static library

CC := clang
CXX := clang++ -std=c++17 -stdlib=libc++

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR_BASE ?= ../build/jdwm_backend

DEBUG_BUILD_DIR := $(BUILD_DIR_BASE)/debug
PROFILE_BUILD_DIR := $(BUILD_DIR_BASE)/profile
RELEASE_BUILD_DIR := $(BUILD_DIR_BASE)/release

DEBUG_LIB := $(DEBUG_BUILD_DIR)/libzenith_backend.a
PROFILE_LIB := $(PROFILE_BUILD_DIR)/libzenith_backend.a
RELEASE_LIB := $(RELEASE_BUILD_DIR)/libzenith_backend.a

SRCS := $(shell find $(SRC_DIR) -name '*.cpp' -or -name '*.cc' -or -name '*.c')

DEBUG_OBJS := $(SRCS:%=$(DEBUG_BUILD_DIR)/%.o)
PROFILE_OBJS := $(SRCS:%=$(PROFILE_BUILD_DIR)/%.o)
RELEASE_OBJS := $(SRCS:%=$(RELEASE_BUILD_DIR)/%.o)

DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)
PROFILE_DEPS := $(PROFILE_OBJS:.o=.d)
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I$(INCLUDE_DIR)

ASAN := -g -fno-omit-frame-pointer -fsanitize=address
WARNINGS := -Wall -Wextra -Werror \
			-Wno-unused-parameter -Wno-unused-variable -Wno-invalid-offsetof -Wno-unknown-pragmas \
			-Wno-deprecated-declarations
BUILD_VERSION ?= local
BUILD_GIT_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo nogit)
BUILD_TIMESTAMP ?= $(shell date -u +%Y-%m-%dT%H:%M:%SZ)

WLR_INC_SUBDIR ?= wlroots-0.19

COMMON_CPPFLAGS := $(INC_FLAGS) \
	 $(WARNINGS) \
	 `pkg-config --cflags pixman-1 libdrm` \
	 -MMD -MP -DWLR_USE_UNSTABLE \
	 -DZENITH_BUILD_VERSION=\"$(BUILD_VERSION)\" \
	 -DZENITH_BUILD_GIT_COMMIT=\"$(BUILD_GIT_COMMIT)\" \
	 -DZENITH_BUILD_TIMESTAMP=\"$(BUILD_TIMESTAMP)\"
ifdef WLR_ROOT
COMMON_CPPFLAGS += -I$(WLR_ROOT)/include -I$(WLR_ROOT)/include/$(WLR_INC_SUBDIR)
endif
ifdef WLR_SRC_ROOT
COMMON_CPPFLAGS += -I$(WLR_SRC_ROOT)/include
COMMON_CPPFLAGS += -I$(WLR_SRC_ROOT)/build/protocol
endif

DEBUG_CPPFLAGS := $(COMMON_CPPFLAGS) -DDEBUG $(ASAN)
PROFILE_CPPFLAGS := $(COMMON_CPPFLAGS) -DPROFILE
RELEASE_CPPFLAGS := $(COMMON_CPPFLAGS) -O2

.PHONY: debug profile release clean

all: debug

debug: $(DEBUG_LIB)

profile: $(PROFILE_LIB)

release: $(RELEASE_LIB)

$(DEBUG_LIB): $(DEBUG_OBJS)
	mkdir -p $(dir $@)
	ar rcs $@ $^

$(PROFILE_LIB): $(PROFILE_OBJS)
	mkdir -p $(dir $@)
	ar rcs $@ $^

$(RELEASE_LIB): $(RELEASE_OBJS)
	mkdir -p $(dir $@)
	ar rcs $@ $^

$(DEBUG_BUILD_DIR)/%.c.o: %.c Makefile
	mkdir -p $(dir $@)
	$(CC) $(DEBUG_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PROFILE_BUILD_DIR)/%.c.o: %.c Makefile
	mkdir -p $(dir $@)
	$(CC) $(PROFILE_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(RELEASE_BUILD_DIR)/%.c.o: %.c Makefile
	mkdir -p $(dir $@)
	$(CC) $(RELEASE_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(DEBUG_BUILD_DIR)/%.cpp.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

$(PROFILE_BUILD_DIR)/%.cpp.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(PROFILE_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

$(RELEASE_BUILD_DIR)/%.cpp.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(RELEASE_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

$(DEBUG_BUILD_DIR)/%.cc.o: %.cc Makefile
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

$(PROFILE_BUILD_DIR)/%.cc.o: %.cc Makefile
	mkdir -p $(dir $@)
	$(CXX) $(PROFILE_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

$(RELEASE_BUILD_DIR)/%.cc.o: %.cc Makefile
	mkdir -p $(dir $@)
	$(CXX) $(RELEASE_CPPFLAGS) -stdlib=libc++ $(CXXFLAGS) -c $< -o $@

-include $(DEBUG_DEPS)
-include $(PROFILE_DEPS)
-include $(RELEASE_DEPS)

clean:
	rm -rf $(BUILD_DIR_BASE)
