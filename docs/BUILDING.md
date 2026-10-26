# Building Íris Emulator

Use CMake 3.16 or newer and a C++17 compiler. Keep the compiler architecture consistent with Qt and SDL.

## Jaguar regression tests without the GUI

```sh
cmake -S . -B build-core -DIRIS_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The real Jaguar processors and peripherals are linked in `cmake/JaguarCore.cmake`. The generated 68000 C sources in `JAGUAR REFERENCE 1/src/m68000/obj` are source inputs, not disposable build output. Never replace the core with the legacy link-only stubs.

## Windows GUI

Dependencies: Qt 6 (Widgets, Svg, Network, OpenGL, OpenGLWidgets), SDL2, Zlib, Discord RPC and the Gearlynx sources. Gearlynx must include its `platforms/shared/dependencies/miniz` sources. The tested toolchain is Qt 6.8.3 with MinGW 13.1 x64 and SDL 2.30.9 x64.

Example (replace paths with your installations):

```powershell
cmake -S . -B build-gui -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64 `
  -DSDL2_DIR=C:/deps/SDL2/cmake `
  -DGEARLYNX_INCLUDE_DIR=C:/deps/Gearlynx/src `
  -DDISCORD_RPC_INCLUDE_DIR=C:/deps/discord/include `
  -DDISCORD_RPC_LIB=C:/deps/discord/lib/discord-rpc.lib `
  -DZLIB_INCLUDE_DIR=C:/deps/zlib/include `
  -DZLIB_LIBRARY=C:/deps/zlib/lib/zlibstatic.lib
cmake --build build-gui --target irisemulator test_jaguar test_jaguar_input test_frame_pacer
ctest --test-dir build-gui --output-on-failure
windeployqt --release --no-translations --compiler-runtime build-gui/irisemulator.exe
```

For MinGW use compatible `.a` import/static libraries for Discord RPC and Zlib in place of the `.lib` examples. Copy matching SDL2.dll and discord-rpc.dll into the output directory; include the `data/mappers.json` file and Qt plugin directories when packaging. Do not package user EEPROM, configuration, ROMs or logs.

`tools/build_local_windows.ps1` is a convenience script for the local `.build-deps` layout used during this repair, not an automatic dependency installer. It requires a configured `build_interface` directory. For reproducible general builds, configure your dependency paths explicitly as above.

Optional ROM checks require a legally supplied local ROM. Example:

```powershell
./build-gui/irisemulator.exe --smoke-test "C:/ROMs/game.jag" --frames 600 --paced --audio-test --output "C:/temp/iris-test"
```

This uses the actual Qt frame loop with isolated settings and writes results, captures and a bounded PCM WAV. For unattended audio-free opening audits, omit `--audio-test`. Python audit tools need Pillow (`pip install Pillow`). Passing an opening audit is not a compatibility certificate.
