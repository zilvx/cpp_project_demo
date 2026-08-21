# AGENTS.md

Qt6 + CMake + Conan C++ desktop app (table data, waveform charting, HTTP backend). See `CLAUDE.md` for full architecture; this file captures the operational gotchas agents miss.

## Build (order matters)

```bash
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/build/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- `conan install` must run first — it generates the toolchain and `Find*.cmake` deps. Because of `[layout] cmake_layout`, these land in a **nested** `.../Release/generators/` dir, not `build/`. Locate with `find . -name conan_toolchain.cmake` if the exact path differs.
- CMakeLists.txt no longer `include()`s the toolchain — it is passed via `-DCMAKE_TOOLCHAIN_FILE`. README/CLAUDE omit this flag.
- Qt6 resolves from `/opt/homebrew/opt/qt` (hardcoded `CMAKE_PREFIX_PATH` prepend).

## Targets

- `qt_table_app` — Qt UI app (macOS `.app` bundle)
- `table_data_server` — standalone HTTP backend (`./build/table_data_server [port]`)
- `table_tests` — GoogleTest (`./build/tests/table_tests` or `cd build && ctest`)

## Gotchas

- **Adding a source file**: register it in BOTH the root `CMakeLists.txt` (app target) AND `tests/CMakeLists.txt` `SRCS_UNDER_TEST` — tests compile sources directly, there is no shared library.
- **Data source is compile-time and mutually exclusive**: `-DUSE_HTTP_DATA=ON` (JSON) or `-DUSE_PROTO_DATA=ON` (Protobuf); default is local `SampleDataProvider`. Setting both is a hard `FATAL_ERROR`.
- **Protobuf codegen**: `USE_PROTO_DATA=ON` invokes `protoc` at configure time, emitting `table_data.pb.{h,cc}` into the build dir (required before AUTOMOC). Schema: `src/utils/proto/table_data.proto`.
- **macOS 26+ AGL**: Qt links `-framework AGL`, which no longer exists. `stub_frameworks/AGL.framework` is a stub; `table_tests` links it via `-F` + `-Wl,-rpath`.

## Conventions

- Code comments, commit messages, and QSS labels are written in **Chinese**.
- Styles belong in `src/ui/styles/*.qss` (loaded via `resources.qrc`), never inline C++ string literals.
- Logging via `LogUtil::init(log_name)` (spdlog: console + rotating file, 5MB × 3).
  - **All processes archive under project-root `logs/`** (walk up from exe/CWD until `CMakeLists.txt` or `.git` is found). Not CWD-relative, not `build/logs/`.
  - Each process uses its own file: `logs/qt_table_app.log`, `logs/table_data_server.log`, `logs/fetch_table_child.log`, …. Prefork workers: `logs/table_data_server_prefork_worker_<pid>.log` (O_APPEND, not parent spdlog).
  - If project root cannot be detected, falls back to exe-adjacent / CWD `logs/`, then temp. File sink failure → console-only + `/tmp/cpp_project_demo_logutil_error.txt`.
  - Call `init()` early and `shutdown()` at exit. Child processes whose stdout is a protocol pipe must pass `use_stderr=true`.
