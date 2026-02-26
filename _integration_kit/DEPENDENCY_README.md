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
git submodule add https://github.com/JappeOS/jdwm.git vendor/jdwm
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

## 4) Add a Makefile

Copy the Makefile from `_integration_kit/Makefile` from this repository to your project.

## 5) Create `src/main.cpp`

```cpp
#include <getopt.h>

#include "zenith_backend/zenith_backend.hpp"

extern "C" {
#define static
#include <wlr/util/log.h>
#undef static
}

int main(int argc, char* argv[]) {
  wlr_log_init(WLR_DEBUG, nullptr);

  while ((getopt(argc, argv, "")) != -1);
  const char* startup_cmd = optind == argc ? "" : argv[optind];
  return zenith_backend_run(startup_cmd);
}
```

## 6) Add scripts

Copy all the *.sh files from `_integration_kit/*` from this repository to your project.

`patch_shadcn_flutter_cache.sh` is temporary, and only needed if using the `shadcn_flutter`
package.

## 7) Add Dockerfile

Copy the Dockerfile (`Dockerfile.zenith-wlroots`) from `_integration_kit/*` from this
repository to your project.

## 8) Add `build_config.env`

Add a file named `build_config.env` to the root of your project, with the following content:
```ini
APP_NAME=<flutter_project_name>
PROJECT_DIR=<flutter_project_name>
```
Replace "<flutter_project_name>" with the name of your Flutter project, specified inside your
`pubspec.yaml` file.

## 9) Add necessary dependencies

1. Download `wlroots` and `wlroots-install` v.0.19 from git to the `vendor/*` directory of your
project.

2. Clone the correct version of Flutter as `flutter_clone` to the `vendor/*` directory of your
project. To find the correct directory, check the Sony embedder version being used from the
Makefile, and then check it's Flutter SDK version. The program will fail it runtime if there's
a version mismatch.

## 10) VS Code launch configs (optional)

Copy the `_integration_kit/.vscode/` directory into the root of your project so Debug/Profile/Release launch use the Sony embedder.

## 11) Build and run

```bash
./run_build.sh --run
```

That produces runnable bundles under `build/jappeos/<arch>/<debug|profile|release>/bundle/`.
