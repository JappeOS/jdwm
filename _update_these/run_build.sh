#!/usr/bin/env bash
# Docker-first build helper for jdwm_flutter_test (Sony embedder + jdwm backend).
# Usage:
#   ./run_build.sh build-image   # build/rebuild shared builder/runtime images
#   ./run_build.sh               # docker release build + export ./release
#   ./run_build.sh fast          # docker release build, skip flutter pub get
#   ./run_build.sh local         # local (non-docker) release build
#   ./run_build.sh debug         # docker debug bundle
#   ./run_build.sh profile       # docker profile bundle

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${SCRIPT_DIR}"

MODE="${1:-release}"
FLUTTER_BIN="${FLUTTER_BIN:-${ROOT_DIR}/flutter_clone/bin/flutter}"
WLR_ROOT="${WLR_ROOT:-${ROOT_DIR}/wlroots-install}"
WLR_SRC_ROOT="${WLR_SRC_ROOT:-${ROOT_DIR}/wlroots}"
RELEASE_DIR="release"
TARGET="jdwm_flutter_test"
BUNDLE_DIR="build/${TARGET}/release/bundle"
BUILDER_IMAGE="zenith-wlroots-builder"
RUNTIME_IMAGE="zenith-wlroots"

usage() {
  cat <<'EOF'
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
    Docker debug bundle in build/jdwm_flutter_test/debug/bundle.

  ./run_build.sh profile
    Docker profile bundle in build/jdwm_flutter_test/profile/bundle.

Environment:
  FLUTTER_BIN   Flutter executable (default: <repo>/flutter_clone/bin/flutter)
  WLR_ROOT      Path to wlroots installation (default: <repo>/wlroots-install)
  WLR_SRC_ROOT  Path to wlroots source tree (default: <repo>/wlroots)
EOF
}

ensure_flutter_clone() {
  if [[ ! -x "${FLUTTER_BIN}" ]]; then
    (cd "${ROOT_DIR}" && ./ensure_flutter_clone.sh)
  fi
  if [[ ! -x "${FLUTTER_BIN}" ]]; then
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
  (cd "${ROOT_DIR}" && ./ensure_flutter_clone.sh)
  docker build -f "${SCRIPT_DIR}/Dockerfile.zenith-wlroots" --target builder -t "${BUILDER_IMAGE}" "${ROOT_DIR}"
  docker build -f "${SCRIPT_DIR}/Dockerfile.zenith-wlroots" -t "${RUNTIME_IMAGE}" "${ROOT_DIR}"
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
    cd /work/jdwm_flutter_test
    export LDFLAGS='-L/usr/local/lib -L/usr/local/lib/x86_64-linux-gnu -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/local/lib/x86_64-linux-gnu'
    export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:/usr/local/share/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:\${PKG_CONFIG_PATH:-}
    make ${build_target} WLR_ROOT=/usr/local WLR_SRC_ROOT=/work/wlroots
  "
}

sync_release() {
  if [[ ! -x "${BUNDLE_DIR}/${TARGET}" ]]; then
    echo "Missing release binary: ${BUNDLE_DIR}/${TARGET}"
    exit 1
  fi

  rm -rf "${RELEASE_DIR}"
  mkdir -p "${RELEASE_DIR}/lib"
  cp -f "${BUNDLE_DIR}/${TARGET}" "${RELEASE_DIR}/${TARGET}"
  cp -f "${BUNDLE_DIR}/lib/libapp.so" "${RELEASE_DIR}/lib/libapp.so"
  cp -f "${BUNDLE_DIR}/lib/libflutter_engine.so" "${RELEASE_DIR}/lib/libflutter_engine.so"
  cp -r "${BUNDLE_DIR}/data" "${RELEASE_DIR}/data"

  # Prefer wlroots from docker image (built with xwayland enabled).
  local have_wlroots=0
  if docker image inspect "${RUNTIME_IMAGE}" >/dev/null 2>&1; then
    local runtime_cid=""
    runtime_cid="$(docker create "${RUNTIME_IMAGE}")"
    if docker cp "${runtime_cid}:/app/zenith/lib/libwlroots-0.19.so" "${RELEASE_DIR}/lib/libwlroots-0.19.so" >/dev/null 2>&1; then
      have_wlroots=1
    fi
    docker rm "${runtime_cid}" >/dev/null
  fi

  # Fallback to local wlroots-install for local (non-docker) builds.
  if (( have_wlroots == 0 )); then
    local wlroots_lib=""
    if [[ -f "${WLR_ROOT}/lib/x86_64-linux-gnu/libwlroots-0.19.so" ]]; then
      wlroots_lib="${WLR_ROOT}/lib/x86_64-linux-gnu/libwlroots-0.19.so"
    elif [[ -f "${WLR_ROOT}/lib/libwlroots-0.19.so" ]]; then
      wlroots_lib="${WLR_ROOT}/lib/libwlroots-0.19.so"
    fi
    if [[ -n "${wlroots_lib}" ]]; then
      cp -f "${wlroots_lib}" "${RELEASE_DIR}/lib/libwlroots-0.19.so"
      have_wlroots=1
    fi
  fi

  if (( have_wlroots == 0 )); then
    echo "Missing libwlroots-0.19.so (neither docker image nor ${WLR_ROOT})"
    exit 1
  fi

  cat > "${RELEASE_DIR}/run_${TARGET}.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${DIR}/lib:${LD_LIBRARY_PATH:-}"
export PATH="${DIR}/bin:${PATH}"
exec "${DIR}/jdwm_flutter_test" "$@"
SCRIPT
  chmod +x "${RELEASE_DIR}/run_${TARGET}.sh"

  echo "Release exported to ${RELEASE_DIR}/"
}

copy_runtime_deps_from_image() {
  if ! docker image inspect "${RUNTIME_IMAGE}" >/dev/null 2>&1; then
    return
  fi

  local cid=""
  cid="$(docker create "${RUNTIME_IMAGE}")"
  for lib in libliftoff.so.0 libdisplay-info.so.2 libwayland-server.so.0 libwayland-client.so.0 libpixman-1.so.0 libdrm.so.2 libseat.so.1 libX11-xcb.so.1 libxcb.so.1 libxcb-composite.so.0 libxcb-render.so.0 libxcb-res.so.0 libxcb-xfixes.so.0 libxcb-icccm.so.4 libxcb-ewmh.so.2; do
    docker cp "${cid}:/app/zenith/lib/${lib}" "${RELEASE_DIR}/lib/${lib}" >/dev/null 2>&1 || true
  done
  mkdir -p "${RELEASE_DIR}/bin"
  docker cp "${cid}:/app/zenith/bin/Xwayland" "${RELEASE_DIR}/bin/Xwayland" >/dev/null 2>&1 || \
    docker cp "${cid}:/usr/bin/Xwayland" "${RELEASE_DIR}/bin/Xwayland" >/dev/null 2>&1 || true
  if [[ -f "${RELEASE_DIR}/bin/Xwayland" ]]; then
    chmod +x "${RELEASE_DIR}/bin/Xwayland" || true
  fi
  docker rm "${cid}" >/dev/null
}

case "${MODE}" in
  build-image)
    docker_build_images
    ;;
  release|"")
    ensure_wlroots
    ensure_flutter_clone
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
    ensure_flutter_clone
    setup_linker_paths
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    make release_bundle FLUTTER="${FLUTTER_BIN}" WLR_ROOT="${WLR_ROOT}" WLR_SRC_ROOT="${WLR_SRC_ROOT}"
    sync_release
    ;;
  debug)
    ensure_wlroots
    ensure_flutter_clone
    ensure_builder_image
    rm -rf build/linux
    "${FLUTTER_BIN}" pub get
    patch_shadcn_flutter_cache
    "${FLUTTER_BIN}" build linux --debug
    run_docker_build debug_bundle_no_flutter
    ;;
  profile)
    ensure_wlroots
    ensure_flutter_clone
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
