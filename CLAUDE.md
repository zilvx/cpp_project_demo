# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt6 + CMake + Conan C++ desktop application with:
- Table data management with virtual keyboard editing
- Waveform charting (oscilloscope style, Catmull-Rom smoothing)
- HTTP backend (JSON/Protobuf) with cpp-httplib
- Prefork server architecture for production HTTP service
- Multi-process architecture: Qt app can fetch data via child process (forked network request)

**Important**: All comments, commit messages, and QSS labels must be in **Chinese**.

---

## Build Commands

### Build Order (CRITICAL)

```bash
# 1. Install Conan dependencies (generates toolchain)
conan install . --output-folder=. --build=missing -s build_type=Release

# 2. Configure CMake (use generated toolchain)
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=build/build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build
```

**Why this order?** Conan's `cmake_layout` places toolchain files in a nested `.../Release/generators/` directory. The root `CMakeLists.txt` only loads the toolchain if not already provided; command-line flag is required.

### Build Targets

| Target | Description | Command |
|--------|-------------|---------|
| `qt_table_app` | Qt UI app (macOS `.app` bundle) | `cmake --build build --target qt_table_app` |
| `qt_table_app_fork` | Qt app with forked data fetching | `cmake --build build --target qt_table_app_fork` |
| `table_data_server` | HTTP backend (cpp-httplib) | `cmake --build build --target table_data_server` |
| `table_data_server_prefork` | Prefork HTTP server (POSIX only) | `cmake --build build --target table_data_server_prefork` |
| `fetch_table_child` | Child process for forked fetching | `cmake --build build --target fetch_table_child` |
| `table_tests` | Unit tests (GoogleTest) | `cmake --build build --target table_tests` |
| `main` | Console test program | `cmake --build build --target main` |

### Data Source Selection (Mutually Exclusive)

```bash
# Default: local sample data
cmake -S . -B build

# HTTP JSON backend
cmake -S . -B build -DUSE_HTTP_DATA=ON

# HTTP Protobuf backend
cmake -S . -B build -DUSE_PROTO_DATA=ON
```

Cannot set both `USE_HTTP_DATA` and `USE_PROTO_DATA` simultaneously (CMakeLists.txt:27-29).

### Run Applications

```bash
# Qt app
open build/qt_table_app.app

# HTTP server (default port 8080)
./build/table_data_server 8080

# Run a single test
./build/tests/table_tests --gtest_filter=WaveformDataTest.*

# Run all tests via ctest
cd build && ctest --output-on-failure
```

---

## Architecture

### Data Provider Pattern (Compile-Time Selection)

Three mutually exclusive data providers, all implementing `IDataProvider`:

```
IDataProvider (QObject)
    ├── SampleDataProvider    # Local hardcoded data (default)
    ├── HttpDataProvider       # GET /api/table → JSON
    └── HttpProtoDataProvider  # GET /api/table/proto → Protobuf
```

All providers emit `dataReady(TableData)` signal → `TableWidget::loadData()`.

**Key files**: `src/ui/providers/IDataProvider.{h,cpp}`, `SampleDataProvider.{h,cpp}`, `HttpDataProvider.{h,cpp}`, `HttpProtoDataProvider.{h,cpp}`

### Multi-Process Architecture

Qt app can fetch data via separate child process (`fetch_table_child`), avoiding Qt event loop blocking during network I/O:

```
Qt UI (main_ui.cpp)
    └── ForkedHttpDataProvider (uses QProcess)
        └── fetch_table_child (network request + JSON parsing, std::cout/std::cerr protocol)
```

**Key files**: `src/child/fetch_table_child.cpp`, `src/ui/providers/ForkedHttpDataProvider.{h,cpp}`, `src/ui/main_forked_demo.cpp`

### HTTP Backend (Two Implementations)

1. **TableDataServer** (cpp-httplib):
   - Single-threaded server (event loop)
   - Endpoints: `GET /api/table`, `GET /api/table/proto`, `GET /health`
   - Supports JSON and Protobuf based on build config

2. **PreforkServer** (POSIX fork, no Qt/cpp-httplib):
   - Multi-process HTTP server (master + worker pools)
   - Each worker logs to `logs/table_data_server_prefork_worker_<pid>.log`
   - Implements `O_APPEND` per-process log writing

**Key files**: `src/bs/TableDataServer.{h,cpp}`, `src/bs/PreforkServer.{h,cpp}`, `src/bs/main_bs.cpp`, `src/bs/main_prefork.cpp`

### Waveform Charting

- Oscilloscope-style chart with Catmull-Rom spline interpolation
- Supports: sine, square, sawtooth, CSV import
- Draggable markers on curve
- Auto-scaling grid (NiceStep algorithm for tick spacing)

**Key files**: `src/ui/core/WaveformData.{h,cpp}`, `src/ui/core/WaveformGenerator.cpp`, `src/ui/widgets/WaveformChart.{h,cpp}`

### Table Editing

- `EditController`: state machine for cell editing (append/char/backspace/tab/confirm/cancel)
- `VirtualKeyboard`: 98-key mechanical keyboard QSS styling
- `PenIconDelegate`: visual indicator (✎) for editable cells
- Styles in `src/ui/styles/*.qss` (loaded via resources.qrc)

**Key files**: `src/ui/core/EditController.{h,cpp}`, `src/ui/widgets/VirtualKeyboard.{h,cpp}`, `src/ui/core/PenIconDelegate.{h,cpp}`, `src/ui/core/ScrollBar.{h,cpp}`

---

## Testing

### Test Coverage (28 tests, 5 suites)

| Suite | Tests | Coverage |
|-------|-------|----------|
| `NiceStepTest` | 4 | Tick spacing algorithm |
| `WaveformDataTest` | 6 | Waveform generation |
| `WaveformChartTest` | 4 | Coordinate transform, interpolation |
| `TableDataTest` | 6 | Data structure, Protobuf round-trip |
| `EditControllerTest` | 8 | Full state machine branches |

**Important**: Test sources (`SRCS_UNDER_TEST`) are compiled directly into the test binary (no shared libraries). When adding new source files, register them in both root `CMakeLists.txt` (app target) AND `tests/CMakeLists.txt` `SRCS_UNDER_TEST` (lines 16-22).

### Running Tests

```bash
# Build tests
cmake --build build --target table_tests

# Run all tests
./build/tests/table_tests

# Run specific test suite
./build/tests/table_tests --gtest_filter=NiceStepTest.*

# Run specific test case
./build/tests/table_tests --gtest_filter=EditControllerTest.TestCharAppend/0

# Run via ctest
cd build && ctest --output-on-failure
```

---

## Dependencies

### Conan Packages

- `spdlog/1.13.0` — Logging
- `protobuf/3.21.12` — Serialization (includes protoc + abseil)
- `gtest/1.14.0` — Testing framework
- `cpp-httplib/0.47.0` — HTTP library

### System Dependencies

- **Qt6** (Widgets + Network) — Installed via Homebrew at `/opt/homebrew/opt/qt`
- **CMake** (>= 3.10)
- **Conan** (>= 1.50)

### macOS 26+ AGL Compatibility

Qt 6 links to `-framework AGL`, which no longer exists in macOS 26+. Project provides a stub framework in `stub_frameworks/AGL.framework`:

- Linked via `-F${CMAKE_SOURCE_DIR}/stub_frameworks -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks`
- Applied to all targets requiring Qt (qt_table_app, qt_table_app_fork, table_tests)
- Ensure both `-F` and `-Wl,-rpath` are used together

---

## Logging

**All processes** (Qt app, server, child process, prefork workers) log to project root `logs/`:

- `logs/qt_table_app.log`
- `logs/table_data_server.log`
- `logs/fetch_table_child.log`
- `logs/table_data_server_prefork_worker_<pid>.log` (per-worker)

**Config**: `LogUtil::init(log_name)` uses spdlog with:
- Console output + rotating file (5MB × 3 files)
- Auto-detect project root by walking up from exe/CWD until `CMakeLists.txt` or `.git` is found
- Falls back to CWD-relative `logs/`, then temp directory
- File sink failure → console-only with fallback error log

**Child processes** with stdout redirected to protocol pipe must call `init(..., use_stderr=true)`.

**Shutdown**: Call `LogUtil::shutdown()` at exit for proper cleanup.

---

## Packaging

**Script**: `packaging/build_package.py` — Cross-platform build-and-packaging script.

### Usage

```bash
# Full build + package current platform
python3 packaging/build_package.py

# Package only Qt app
python3 packaging/build_package.py -t qt

# Package only server
python3 packaging/build_package.py -t server

# Build with HTTP JSON data source
python3 packaging/build_package.py --data-source http

# Skip CMake build, package existing artifacts
python3 packaging/build_package.py --skip-build
```

### Target Configuration

| Target | Contents |
|--------|----------|
| `all` (default) | Qt app + server + console program |
| `qt` | `qt_table_app`, `qt_table_app_fork` (includes embedded child process) |
| `server` | `table_data_server`, `table_data_server_prefork` |
| `console` | `main` |

### Platform-Specific Notes

- **macOS**: DMG contains `.app` bundles with embedded Qt frameworks and ad-hoc signature
- **Windows**: Requires MSVC developer environment (`vcvars64`); use `--qt-dir` to specify Qt installation
- **Linux**: Requires `patchelf` for rpath relocation; script warns if missing

---

## Conventions & Gotchas

### Code Comments & Labels

- **All documentation, comments, and QSS labels must be in Chinese** (AGENTS.md:30-33)

### Resource Loading

All QSS styles are defined in `src/ui/styles/*.qss` and loaded via `src/ui/resources.qrc`. QSS must never be embedded as C++ string literals.

**Key resource files**:
- `resources.qrc` — Qt resource collection
- `src/ui/styles/table.qss`, `keyboard_style.qss`, `scrollbar_style.qss`

### CMake Conventions

- **Adding source files**: Register in both root `CMakeLists.txt` (app target) AND `tests/CMakeLists.txt` `SRCS_UNDER_TEST`
- **Protobuf codegen**: `USE_PROTO_DATA=ON` invokes `protoc` at configure time, emits `table_data.pb.{h,cc}` to build dir (required before AUTOMOC)
- **Qt path**: Qt6 resolved from hardcoded `/opt/homebrew/opt/qt` (`CMAKE_PREFIX_PATH` prepend)

### Prefork Worker Logging

Each worker logs to its own file (`logs/table_data_server_prefork_worker_<pid>.log`). Must use `O_APPEND` (not parent spdlog) because:
- Workers inherit parent file descriptor but share inode
- Multiple processes writing to same file descriptor without `O_APPEND` causes interleaved corruption

**See**: `src/bs/PreforkServer.cpp` log setup.

### Child Process Logging Protocol

When `fetch_table_child` stdout is redirected to a protocol pipe:
- Child must call `LogUtil::init(..., use_stderr=true)` to avoid log collisions
- Protocol: `std::cout` for JSON, `std::cerr` for errors/protocol messages
- Parent (`ForkedHttpDataProvider`) reads from pipe and parses output

---

## Debugging & Troubleshooting

### Build Failures

**Conan toolchain not found**: Ensure `conan install` ran first. Toolchain files are generated in `build/build/build/Release/generators/` (nested layout).

**Qt framework not found**: Qt6 must be installed at `/opt/homebrew/opt/qt`. Verify with `brew list qt6`.

**macOS AGL framework error**: Check that `stub_frameworks/AGL.framework` exists and is properly linked. Verify `-F` and `-Wl,-rpath` flags are set together.

### Runtime Issues

**Qt app crashes on startup**: Check logs in `logs/` directory. Ensure resources.qrc is properly loaded (AUTOMOC handles this automatically).

**Child process fails to communicate**: Verify child process calls `LogUtil::init(..., use_stderr=true)` when stdout is redirected. Check stderr contains protocol messages.

**Prefork workers not logging**: Verify each worker uses `O_APPEND` mode (see `PreforkServer.cpp`). Workers logging to same file descriptor without `O_APPEND` corrupt log output.

**Protobuf errors at runtime**: Ensure `USE_PROTO_DATA=ON` is set during both cmake and runtime. Generated files must exist in build dir (`table_data.pb.{h,cc}`).

---

## External Documentation

- `AGENTS.md` — Operational gotchas and build details (read first)
- `README.md` — Chinese user-facing documentation with quick start
- `docs/troubleshooting.md` — 14 historical issues and solutions
- `docs/optimization.md` — 20 code quality optimizations
- `docs/arb_widget_design.md` — Arbitrary waveform widget design docs
- `docs/arb_widget_redesign.md` — Arbitrary waveform widget redesign docs
- `AGENTS.md` — Operational gotchas (build order, data source mutuality, AGL compatibility, Chinese convention requirement)
