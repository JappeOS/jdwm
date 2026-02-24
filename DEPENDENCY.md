# Using `jdwm` in a New Flutter Project (Git Submodule Setup)

This package is not a pure Dart dependency. You must add it to `pubspec.yaml` **and** include a Makefile to build/link the native backend and Sony Linux embedder.

## 1) Create a new Flutter project

```bash
flutter create my_compositor
cd my_compositor
```

## 2) Add `jdwm` as a Git submodule

```bash
mkdir -p vendor
git submodule add https://github.com/<your-org>/<your-repo>.git vendor/jdwm
```

## 3) Add a path dependency in `pubspec.yaml`

```yaml
dependencies:
  jdwm:
    path: vendor/jdwm
```

Then run:

```bash
flutter pub get
```

## 4) Add a Makefile (required)

Create a `Makefile` in your app root with the following content and update:
- `TARGET_EXEC` to your app name.
- `BACKEND_DIR` if your submodule path differs.

```makefile
# Makefile for JDWM Flutter compositor bundle (Sony embedder + jdwm backend)

CC := clang
CXX := clang++ -std=c++17 -stdlib=libc++
FLUTTER := flutter

uname_m = $(shell uname -m)
ifeq ($(uname_m),x86_64)
ARCH := x64
else
ARCH := arm64
endif

TARGET_EXEC := my_compositor
SRC_DIRS := src
DEPS_DIR := deps
BACKEND_DIR := vendor/jdwm/native

DEBUG_BUILD_DIR := build/jappeos/$(ARCH)/debug
PROFILE_BUILD_DIR := build/jappeos/$(ARCH)/profile
RELEASE_BUILD_DIR := build/jappeos/$(ARCH)/release

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
EOF
```

## 5) Add `run_build.sh`

Create `run_build.sh` in your app root and make it executable:

```bash
chmod +x run_build.sh
```

```bash
#!/usr/bin/env bash
# Docker-first build helper for a Flutter compositor (Sony embedder + jdwm backend).
# Usage:
#   ./run_build.sh build-image   # build/rebuild shared builder/runtime images
#   ./run_build.sh               # docker release build + export ./release
#   ./run_build.sh fast          # docker release build, skip flutter pub get
#   ./run_build.sh local         # local (non-docker) release build
#   ./run_build.sh debug         # docker debug bundle
#   ./run_build.sh profile       # docker profile bundle

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"
cd "${SCRIPT_DIR}"

MODE="${1:-release}"
FLUTTER_BIN="${FLUTTER_BIN:-flutter}"
WLR_ROOT="${WLR_ROOT:-${ROOT_DIR}/wlroots-install}"
WLR_SRC_ROOT="${WLR_SRC_ROOT:-${ROOT_DIR}/wlroots}"
RELEASE_DIR="release"
TARGET="${TARGET:-my_compositor}"
if [[ "$(uname -m)" == "x86_64" ]]; then
  ARCH="x64"
else
  ARCH="arm64"
fi
BUNDLE_DIR="build/jappeos/${ARCH}/release/bundle"
BUILDER_IMAGE="zenith-wlroots-builder"
RUNTIME_IMAGE="zenith-wlroots"

usage() {
  cat <<'USAGE'
Usage:
  ./run_build.sh build-image
    Build/rebuild Docker images used by this script.

  ./run_build.sh
    Docker release build and export runtime bundle to ./release.

  ./run_build.sh fast
    Docker release build only (skip flutter pub get), then export ./release.

  ./run_build.sh local
    Local (non-docker) release build and export ./release.

  ./run_build.sh debug
    Docker debug bundle in build/<target>/debug/bundle.

  ./run_build.sh profile
    Docker profile bundle in build/<target>/profile/bundle.

Environment:
  FLUTTER_BIN   Flutter executable (default: flutter)
  WLR_ROOT      Path to wlroots installation (default: <repo>/wlroots-install)
  WLR_SRC_ROOT  Path to wlroots source tree (default: <repo>/wlroots)
  TARGET        Output binary name (default: my_compositor)
USAGE
}

ensure_flutter() {
  if ! command -v "${FLUTTER_BIN}" >/dev/null 2>&1; then
    echo "Missing Flutter executable: ${FLUTTER_BIN}"
    exit 1
  fi
}

patch_shadcn_flutter_cache() {
  local shadcn_files=""
  shadcn_files="$(find "${HOME}/.pub-cache/git" -maxdepth 4 -type f -path "*/shadcn_flutter-*/lib/shadcn_flutter.dart" 2>/dev/null || true)"
  if [[ -z "${shadcn_files}" ]]; then
    return
  fi

  while IFS= read -r shadcn_file; do
    [[ -z "${shadcn_file}" ]] && continue
    local shadcn_dir
    shadcn_dir="$(dirname "${shadcn_file}")"

    # Flutter 3.32 widgets.dart no longer re-exports Brightness; shadcn expects it.
    if grep -q "show Icons, MaterialPageRoute, MaterialPage, SliverAppBar, FlutterLogo;" "${shadcn_file}"; then
      sed -i "s/show Icons, MaterialPageRoute, MaterialPage, SliverAppBar, FlutterLogo;/show Icons, MaterialPageRoute, MaterialPage, SliverAppBar, FlutterLogo, Brightness;/" "${shadcn_file}"
    fi

    # vector_math 2.1.x doesn't have scaleByDouble/translateByDouble.
    find "${shadcn_dir}/src" -type f -name "*.dart" -print0 | xargs -0 perl -0777 -i -pe \
      's/translateByDouble\(([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)/translate($1, $2, $3)/gs; s/scaleByDouble\(([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)/scale($1, $2, $3)/gs;'
  done <<< "${shadcn_files}"
}

ensure_wlroots() {
  if [[ ! -d "${WLR_ROOT}" ]]; then
    echo "Missing WLR_ROOT: ${WLR_ROOT}"
    exit 1
  fi
  if [[ ! -d "${WLR_SRC_ROOT}" ]]; then
    echo "Missing WLR_SRC_ROOT: ${WLR_SRC_ROOT}"
    exit 1
  fi
}

setup_linker_paths() {
  local extra="-L/usr/local/lib -L/usr/local/lib/x86_64-linux-gnu -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/local/lib/x86_64-linux-gnu"
  if [[ -n "${LDFLAGS:-}" ]]; then
    export LDFLAGS="${LDFLAGS} ${extra}"
  else
    export LDFLAGS="${extra}"
  fi
}

docker_build_images() {
  docker build -f "${ROOT_DIR}/Dockerfile.zenith-wlroots" --target builder -t "${BUILDER_IMAGE}" "${ROOT_DIR}"
  docker build -f "${ROOT_DIR}/Dockerfile.zenith-wlroots" -t "${RUNTIME_IMAGE}" "${ROOT_DIR}"
}

ensure_builder_image() {
  if ! docker image inspect "${BUILDER_IMAGE}" >/dev/null 2>&1; then
    echo "Docker image ${BUILDER_IMAGE} not found. Run: ./run_build.sh build-image"
    exit 1
  fi
}

run_docker_build() {
  local build_target="$1"

  docker run --rm --user "$(id -u):$(id -g)" --entrypoint bash -v "${ROOT_DIR}:/work" "${BUILDER_IMAGE}" -lc "
    set -e
    cd /work
    export LDFLAGS='-L/usr/local/lib -L/usr/local/lib/x86_64-linux-gnu -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/local/lib/x86_64-linux-gnu'
    make ${build_target} WLR_ROOT=/work/wlroots-install WLR_SRC_ROOT=/work/wlroots
  "
}

sync_release() {
  if [[ ! -x "${BUNDLE_DIR}/${TARGET}" ]]; then
    echo "Missing release binary: ${BUNDLE_DIR}/${TARGET}"
    exit 1
  fi

  local wlroots_lib=""
  if [[ -f "${WLR_ROOT}/lib/x86_64-linux-gnu/libwlroots-0.19.so" ]]; then
    wlroots_lib="${WLR_ROOT}/lib/x86_64-linux-gnu/libwlroots-0.19.so"
  elif [[ -f "${WLR_ROOT}/lib/libwlroots-0.19.so" ]]; then
    wlroots_lib="${WLR_ROOT}/lib/libwlroots-0.19.so"
  else
    echo "Missing libwlroots-0.19.so under ${WLR_ROOT}/lib"
    exit 1
  fi

  rm -rf "${RELEASE_DIR}"
  mkdir -p "${RELEASE_DIR}/lib"
  cp -f "${BUNDLE_DIR}/${TARGET}" "${RELEASE_DIR}/${TARGET}"
  cp -f "${BUNDLE_DIR}/lib/libapp.so" "${RELEASE_DIR}/lib/libapp.so"
  cp -f "${BUNDLE_DIR}/lib/libflutter_engine.so" "${RELEASE_DIR}/lib/libflutter_engine.so"
  cp -f "${wlroots_lib}" "${RELEASE_DIR}/lib/libwlroots-0.19.so"
  cp -r "${BUNDLE_DIR}/data" "${RELEASE_DIR}/data"

  cat > "${RELEASE_DIR}/run_${TARGET}.sh" <<RUNSCRIPT
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${DIR}/lib:${LD_LIBRARY_PATH:-}"
exec "${DIR}/${TARGET}" "$@"
RUNSCRIPT
  chmod +x "${RELEASE_DIR}/run_${TARGET}.sh"

  echo "Release exported to ${RELEASE_DIR}/"
}

copy_runtime_deps_from_image() {
  if ! docker image inspect "${RUNTIME_IMAGE}" >/dev/null 2>&1; then
    return
  fi

  local cid=""
  cid="$(docker create "${RUNTIME_IMAGE}")"
  for lib in libliftoff.so.0 libdisplay-info.so.2 libwayland-server.so.0 libwayland-client.so.0 libpixman-1.so.0 libdrm.so.2 libseat.so.1; do
    docker cp "${cid}:/app/zenith/lib/${lib}" "${RELEASE_DIR}/lib/${lib}" >/dev/null 2>&1 || true
  done
  docker rm "${cid}" >/dev/null
}

case "${MODE}" in
  build-image)
    docker_build_images
    ;;
  release|"")
    ensure_wlroots
    ensure_flutter
    ensure_builder_image
    rm -rf build/linux
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    "${FLUTTER_BIN}" build linux --release
    run_docker_build release_bundle_no_flutter
    sync_release
    copy_runtime_deps_from_image
    ;;
  fast)
    ensure_wlroots
    ensure_builder_image
    patch_shadcn_flutter_cache
    run_docker_build release_bundle_no_flutter
    sync_release
    copy_runtime_deps_from_image
    ;;
  local)
    ensure_wlroots
    ensure_flutter
    setup_linker_paths
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    make release_bundle FLUTTER="${FLUTTER_BIN}" WLR_ROOT="${WLR_ROOT}" WLR_SRC_ROOT="${WLR_SRC_ROOT}"
    sync_release
    ;;
  debug)
    ensure_wlroots
    ensure_flutter
    ensure_builder_image
    rm -rf build/linux
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    "${FLUTTER_BIN}" build linux --debug
    run_docker_build debug_bundle_no_flutter
    ;;
  profile)
    ensure_wlroots
    ensure_flutter
    ensure_builder_image
    rm -rf build/linux
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    "${FLUTTER_BIN}" build linux --profile
    run_docker_build profile_bundle_no_flutter
    ;;
  help|--help|-h)
    usage
    ;;
  *)
    echo "Unknown mode: ${MODE}"
    usage
    exit 1
    ;;
esac
```

## 6) Add a root runner (`run.sh`)

Create `run.sh` in your app root and make it executable:

```bash
chmod +x run.sh
```

```bash
#!/usr/bin/env bash
# Runner for a Flutter compositor bundle.
# Usage:
#   ./run.sh            # release
#   ./run.sh debug
#   ./run.sh profile
#   ./run.sh release

set -euo pipefail

MODE="${1:-release}"
TARGET="${TARGET:-my_compositor}"

if [[ "$(uname -m)" == "x86_64" ]]; then
  ARCH="x64"
else
  ARCH="arm64"
fi

BUNDLE_DIR="build/jappeos/${ARCH}/${MODE}/bundle"
BIN="${BUNDLE_DIR}/${TARGET}"

if [[ ! -x "${BIN}" ]]; then
  echo "Missing bundle binary: ${BIN}"
  echo "Build it first (for example: ./run_build.sh ${MODE})"
  exit 1
fi

export LD_LIBRARY_PATH="${BUNDLE_DIR}/lib:${LD_LIBRARY_PATH:-}"
exec "${BIN}" "$@"
```

## 7) VS Code launch configs

Add these files under `.vscode/` in your app root so Debug/Profile/Release launch uses the Sony embedder.

`.vscode/tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "jdwm:detect-arch",
      "type": "shell",
      "command": "bash",
      "args": [
        "-lc",
        "mkdir -p .vscode && if [ \"$(uname -m)\" = \"x86_64\" ]; then arch=x64; else arch=arm64; fi; echo JDWM_ARCH=$arch > .vscode/arch.env"
      ]
    },
    {
      "label": "jdwm:build-debug",
      "type": "shell",
      "command": "./run_build.sh",
      "args": [
        "debug"
      ],
      "dependsOn": [
        "jdwm:detect-arch"
      ],
      "problemMatcher": []
    },
    {
      "label": "jdwm:build-profile",
      "type": "shell",
      "command": "./run_build.sh",
      "args": [
        "profile"
      ],
      "dependsOn": [
        "jdwm:detect-arch"
      ],
      "problemMatcher": []
    },
    {
      "label": "jdwm:build-release",
      "type": "shell",
      "command": "./run_build.sh",
      "args": [
        "release"
      ],
      "dependsOn": [
        "jdwm:detect-arch"
      ],
      "problemMatcher": []
    }
  ]
}
```

`.vscode/launch.json`

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "JDWM Debug (Sony)",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/debug/bundle/${input:targetName}",
      "args": [],
      "cwd": "${workspaceFolder}",
      "stopAtEntry": false,
      "environment": [
        {
          "name": "LD_LIBRARY_PATH",
          "value": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/debug/bundle/lib:${env:LD_LIBRARY_PATH}"
        }
      ],
      "MIMode": "gdb",
      "preLaunchTask": "jdwm:build-debug",
      "envFile": "${workspaceFolder}/.vscode/arch.env",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    },
    {
      "name": "JDWM Profile (Sony)",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/profile/bundle/${input:targetName}",
      "args": [],
      "cwd": "${workspaceFolder}",
      "stopAtEntry": false,
      "environment": [
        {
          "name": "LD_LIBRARY_PATH",
          "value": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/profile/bundle/lib:${env:LD_LIBRARY_PATH}"
        }
      ],
      "MIMode": "gdb",
      "preLaunchTask": "jdwm:build-profile",
      "envFile": "${workspaceFolder}/.vscode/arch.env",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    },
    {
      "name": "JDWM Release (Sony)",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/release/bundle/${input:targetName}",
      "args": [],
      "cwd": "${workspaceFolder}",
      "stopAtEntry": false,
      "environment": [
        {
          "name": "LD_LIBRARY_PATH",
          "value": "${workspaceFolder}/build/jappeos/${env:JDWM_ARCH}/release/bundle/lib:${env:LD_LIBRARY_PATH}"
        }
      ],
      "MIMode": "gdb",
      "preLaunchTask": "jdwm:build-release",
      "envFile": "${workspaceFolder}/.vscode/arch.env",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    }
  ],
  "inputs": [
    {
      "id": "targetName",
      "type": "promptString",
      "description": "Binary name (TARGET)",
      "default": "my_compositor"
    }
  ]
}
```

## 8) Build and run

```bash
./run_build.sh
```

That produces a runnable bundle under `release/`.
