# A set of build scripts for compiling C applications to various platforms.

Currently supported platforms: Linux, Android, Emscripten.

The only supported build system is `meson`.

# Usage
- Place your application into the `subprojects` directory.
- Use `scripts/setup_build_env` to specify your target platform, and the
  subproject you want to use.
- The script will print the path to the build directory. You can now use
  `ninja` there to compile your application.

# Platform specific setup

## Android
You will need the Android SDK. Edit `cross_files/android-aarch64.ini` to
specify where your SDK is found. There are also may be some hardcoded SDK
version specific values in `meson.build`.

## Emscripten
You need to have `emscripten` installed.
