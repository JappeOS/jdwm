# Makefile for JDWM Flutter Test compositor bundle (Sony embedder + jdwm backend)

CC := clang
CXX := clang++ -std=c++17 -stdlib=libc++
FLUTTER := flutter
BUILD_CONFIG := build_config.env
-include $(BUILD_CONFIG)
APP_NAME ?= jdwm_flutter_test

uname_m = $(shell uname -m)
ifeq ($(uname_m),x86_64)
ARCH := x64
else
ARCH := arm64
endif

TARGET_EXEC := $(APP_NAME)
SRC_DIRS := src
DEPS_DIR := deps
BACKEND_DIR := vendor/jdwm/native

DEBUG_BUILD_DIR := build/$(TARGET_EXEC)/debug
PROFILE_BUILD_DIR := build/$(TARGET_EXEC)/profile
RELEASE_BUILD_DIR := build/$(TARGET_EXEC)/release

DEBUG_BUNDLE_DIR := $(DEBUG_BUILD_DIR)/bundle
PROFILE_BUNDLE_DIR := $(PROFILE_BUILD_DIR)/bundle
RELEASE_BUNDLE_DIR := $(RELEASE_BUILD_DIR)/bundle

BACKEND_BUILD_DIR := build/jdwm_backend
BACKEND_DEBUG_LIB := $(BACKEND_BUILD_DIR)/debug/libzenith_backend.a
BACKEND_PROFILE_LIB := $(BACKEND_BUILD_DIR)/profile/libzenith_backend.a
BACKEND_RELEASE_LIB := $(BACKEND_BUILD_DIR)/release/libzenith_backend.a

BACKEND_MAKE_ARGS = WLR_ROOT=$(abspath $(WLR_ROOT)) WLR_SRC_ROOT=$(abspath $(WLR_SRC_ROOT)) \
	WLR_INC_SUBDIR=$(WLR_INC_SUBDIR) BUILD_VERSION=$(BUILD_VERSION) \
	BUILD_GIT_COMMIT=$(BUILD_GIT_COMMIT) BUILD_TIMESTAMP=$(BUILD_TIMESTAMP) \
	BUILD_DIR_BASE=$(abspath $(BACKEND_BUILD_DIR))

SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.cc' -or -name '*.c')

DEBUG_OBJS := $(SRCS:%=$(DEBUG_BUILD_DIR)/%.o)
PROFILE_OBJS := $(SRCS:%=$(PROFILE_BUILD_DIR)/%.o)
RELEASE_OBJS := $(SRCS:%=$(RELEASE_BUILD_DIR)/%.o)

DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)
PROFILE_DEPS := $(PROFILE_OBJS:.o=.d)
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I$(BACKEND_DIR)/include

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
endif

DEBUG_CPPFLAGS := $(COMMON_CPPFLAGS) -DDEBUG $(ASAN)
PROFILE_CPPFLAGS := $(COMMON_CPPFLAGS) -DPROFILE
RELEASE_CPPFLAGS := $(COMMON_CPPFLAGS) -O2

COMMON_LDFLAGS := -linput -lwlroots-0.19 -lwayland-server -lwayland-client -lxkbcommon -lpixman-1 -ldrm \
                   -lliftoff -ldisplay-info -lepoxy -lGLESv2 -lEGL -lGL -lpam \
                   -L. -L$(DEPS_DIR)
ifdef WLR_ROOT
COMMON_LDFLAGS += -L$(WLR_ROOT)/lib -L$(WLR_ROOT)/lib/x86_64-linux-gnu -Wl,-rpath,$(WLR_ROOT)/lib -Wl,-rpath,$(WLR_ROOT)/lib/x86_64-linux-gnu
endif

DEBUG_LDFLAGS := $(COMMON_LDFLAGS) -lflutter_engine_debug $(ASAN) -stdlib=libc++ -lc++abi -lpthread
PROFILE_LDFLAGS := $(COMMON_LDFLAGS) -lflutter_engine_profile -stdlib=libc++ -lc++abi -lpthread
RELEASE_LDFLAGS := $(COMMON_LDFLAGS) -lflutter_engine_release -stdlib=libc++ -lc++abi -lpthread

DEBUG_LDFLAGS += -L$(BACKEND_BUILD_DIR)/debug -lzenith_backend
PROFILE_LDFLAGS += -L$(BACKEND_BUILD_DIR)/profile -lzenith_backend
RELEASE_LDFLAGS += -L$(BACKEND_BUILD_DIR)/release -lzenith_backend

$(DEPS_DIR)/libflutter_engine_debug.so:
	curl -L https://github.com/sony/flutter-embedded-linux/releases/download/9064459a8b/elinux-$(ARCH)-debug.zip >/tmp/elinux-$(ARCH)-debug.zip
	unzip -o /tmp/elinux-$(ARCH)-debug.zip -d /tmp || exit
	mkdir -p $(DEPS_DIR)
	mv /tmp/libflutter_engine.so $(DEPS_DIR)/libflutter_engine_debug.so

$(DEPS_DIR)/libflutter_engine_profile.so:
	curl -L https://github.com/sony/flutter-embedded-linux/releases/download/9064459a8b/elinux-$(ARCH)-profile.zip >/tmp/elinux-$(ARCH)-profile.zip
	unzip -o /tmp/elinux-$(ARCH)-profile.zip -d /tmp || exit
	mkdir -p $(DEPS_DIR)
	mv /tmp/libflutter_engine.so $(DEPS_DIR)/libflutter_engine_profile.so

$(DEPS_DIR)/libflutter_engine_release.so:
	curl -L https://github.com/sony/flutter-embedded-linux/releases/download/9064459a8b/elinux-$(ARCH)-release.zip >/tmp/elinux-$(ARCH)-release.zip
	unzip -o /tmp/elinux-$(ARCH)-release.zip -d /tmp || exit
	mkdir -p $(DEPS_DIR)
	mv /tmp/libflutter_engine.so $(DEPS_DIR)/libflutter_engine_release.so

$(DEBUG_BUNDLE_DIR)/$(TARGET_EXEC): $(DEBUG_OBJS) $(DEPS_DIR)/libflutter_engine_debug.so $(BACKEND_DEBUG_LIB)
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_OBJS) -o $@ -Wl,-rpath='$$ORIGIN/lib' $(DEBUG_LDFLAGS) $(LDFLAGS)

$(PROFILE_BUNDLE_DIR)/$(TARGET_EXEC): $(PROFILE_OBJS) $(DEPS_DIR)/libflutter_engine_profile.so $(BACKEND_PROFILE_LIB)
	mkdir -p $(dir $@)
	$(CXX) $(PROFILE_OBJS) -o $@ -Wl,-rpath='$$ORIGIN/lib' $(PROFILE_LDFLAGS) $(LDFLAGS)

$(RELEASE_BUNDLE_DIR)/$(TARGET_EXEC): $(RELEASE_OBJS) $(DEPS_DIR)/libflutter_engine_release.so $(BACKEND_RELEASE_LIB)
	mkdir -p $(dir $@)
	$(CXX) $(RELEASE_OBJS) -o $@ -Wl,-rpath='$$ORIGIN/lib' $(RELEASE_LDFLAGS) $(LDFLAGS)

$(BACKEND_DEBUG_LIB):
	$(MAKE) -C $(BACKEND_DIR) debug $(BACKEND_MAKE_ARGS)

$(BACKEND_PROFILE_LIB):
	$(MAKE) -C $(BACKEND_DIR) profile $(BACKEND_MAKE_ARGS)

$(BACKEND_RELEASE_LIB):
	$(MAKE) -C $(BACKEND_DIR) release $(BACKEND_MAKE_ARGS)

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

flutter_debug:
	$(FLUTTER) build linux --debug

flutter_profile:
	$(FLUTTER) build linux --profile

flutter_release:
	$(FLUTTER) build linux --release

debug_bundle: flutter_debug $(DEBUG_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(DEBUG_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_debug.so $(DEBUG_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -d build/linux/$(ARCH)/debug/bundle/data ]; then cp -r build/linux/$(ARCH)/debug/bundle/data $(DEBUG_BUNDLE_DIR); else cp -r build/flutter_assets $(DEBUG_BUNDLE_DIR)/data; fi
	cp lsan_suppressions.txt $(DEBUG_BUNDLE_DIR)

profile_bundle: flutter_profile $(PROFILE_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(PROFILE_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_profile.so $(PROFILE_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -f build/linux/$(ARCH)/profile/bundle/lib/libapp.so ]; then cp build/linux/$(ARCH)/profile/bundle/lib/libapp.so $(PROFILE_BUNDLE_DIR)/lib; else cp build/lib/libapp.so $(PROFILE_BUNDLE_DIR)/lib; fi
	if [ -d build/linux/$(ARCH)/profile/bundle/data ]; then cp -r build/linux/$(ARCH)/profile/bundle/data $(PROFILE_BUNDLE_DIR); else cp -r build/flutter_assets $(PROFILE_BUNDLE_DIR)/data; fi

release_bundle: flutter_release $(RELEASE_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(RELEASE_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_release.so $(RELEASE_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -f build/linux/$(ARCH)/release/bundle/lib/libapp.so ]; then cp build/linux/$(ARCH)/release/bundle/lib/libapp.so $(RELEASE_BUNDLE_DIR)/lib; else cp build/lib/libapp.so $(RELEASE_BUNDLE_DIR)/lib; fi
	if [ -d build/linux/$(ARCH)/release/bundle/data ]; then cp -r build/linux/$(ARCH)/release/bundle/data $(RELEASE_BUNDLE_DIR); else cp -r build/flutter_assets $(RELEASE_BUNDLE_DIR)/data; fi

debug_bundle_no_flutter: $(DEBUG_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(DEBUG_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_debug.so $(DEBUG_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -d build/linux/$(ARCH)/debug/bundle/data ]; then cp -r build/linux/$(ARCH)/debug/bundle/data $(DEBUG_BUNDLE_DIR); else cp -r build/flutter_assets $(DEBUG_BUNDLE_DIR)/data; fi
	cp lsan_suppressions.txt $(DEBUG_BUNDLE_DIR)

profile_bundle_no_flutter: $(PROFILE_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(PROFILE_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_profile.so $(PROFILE_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -f build/linux/$(ARCH)/profile/bundle/lib/libapp.so ]; then cp build/linux/$(ARCH)/profile/bundle/lib/libapp.so $(PROFILE_BUNDLE_DIR)/lib; else cp build/lib/libapp.so $(PROFILE_BUNDLE_DIR)/lib; fi
	if [ -d build/linux/$(ARCH)/profile/bundle/data ]; then cp -r build/linux/$(ARCH)/profile/bundle/data $(PROFILE_BUNDLE_DIR); else cp -r build/flutter_assets $(PROFILE_BUNDLE_DIR)/data; fi

release_bundle_no_flutter: $(RELEASE_BUNDLE_DIR)/$(TARGET_EXEC)
	mkdir -p $(RELEASE_BUNDLE_DIR)/lib/
	cp $(DEPS_DIR)/libflutter_engine_release.so $(RELEASE_BUNDLE_DIR)/lib/libflutter_engine.so
	if [ -f build/linux/$(ARCH)/release/bundle/lib/libapp.so ]; then cp build/linux/$(ARCH)/release/bundle/lib/libapp.so $(RELEASE_BUNDLE_DIR)/lib; else cp build/lib/libapp.so $(RELEASE_BUNDLE_DIR)/lib; fi
	if [ -d build/linux/$(ARCH)/release/bundle/data ]; then cp -r build/linux/$(ARCH)/release/bundle/data $(RELEASE_BUNDLE_DIR); else cp -r build/flutter_assets $(RELEASE_BUNDLE_DIR)/data; fi

attach_debugger:
	$(FLUTTER) attach --debug-uri=http://127.0.0.1:12345/

all: debug_bundle profile_bundle release_bundle

clean:
	-rm -rf $(DEBUG_BUILD_DIR) $(PROFILE_BUILD_DIR) $(RELEASE_BUILD_DIR) $(BACKEND_BUILD_DIR)

-include $(DEBUG_DEPS)
-include $(PROFILE_DEPS)
-include $(RELEASE_DEPS)
