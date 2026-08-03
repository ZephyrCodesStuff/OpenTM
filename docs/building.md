# Building

Needs a C++20 compiler, CMake 3.25 or newer, and Qt 6.2 or newer. Catch2 3 is needed only for the tests, and CMake fetches it when the system has no compatible copy.

## Windows

Dependencies come from vcpkg, declared in `vcpkg.json` and fetched on first configure. Set `VCPKG_ROOT` first.

```powershell
cmake --preset msvc-debug
cmake --build --preset msvc-debug
ctest --preset msvc-debug
```

`build.bat` wraps configure, build, test, deploy and run in one step - `build.bat release run` builds Release and launches the GUI. Presets are also provided for the Visual Studio generator (`vs2022`), which does not need a Developer shell.

`vcpkg.json` sets `default-features: false` on `qtbase`, so every Qt feature the project uses has to be listed explicitly - including ones Qt normally enables itself. `png` is one of them; without it Qt silently cannot decode PNG images and the icons come up blank.

### Without vcpkg

vcpkg builds Qt from source, which takes the better part of an hour the first time. If you already have a Qt from the official installer, `msvc-release-prebuilt-qt` skips vcpkg entirely and takes Qt from `CMAKE_PREFIX_PATH` instead - Catch2 is fetched by CMake, so there is nothing else vcpkg was providing.

```powershell
cmake --preset msvc-release-prebuilt-qt -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build --preset msvc-release-prebuilt-qt
ctest --preset msvc-release-prebuilt-qt
```

This is also what CI uses, so Windows release builds never wait on a Qt compile. Note that you do not need a Windows machine to produce Windows builds at all - the release workflow builds and packages them on a GitHub runner.

## Linux

The presets use the system Qt 6.

```sh
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel    # Fedora
sudo apt install g++ cmake ninja-build qt6-base-dev            # Debian/Ubuntu

cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

`build.sh` is the equivalent of `build.bat`: `./build.sh release run`.

`linux-debug-vcpkg` takes everything from vcpkg instead of the system, and needs `VCPKG_ROOT`.

### Catch2

No mainstream distro packages Catch2 3 yet - Fedora, Debian and Ubuntu all still ship 2.x, which the tests do not compile against. So CMake looks for a system Catch2 3, and downloads v3.9.1 during configure when it does not find one. A system copy always wins when there is one.

## Configure options

| | |
|---|---|
| `OPENTM_BUILD_TESTS` | `ON`. Turning it off also skips the Catch2 lookup, so a build needs no network. |
| `OPENTM_FETCH_CATCH2` | `ON`. Set `OFF` to require a system Catch2 3 and fail instead of downloading. |