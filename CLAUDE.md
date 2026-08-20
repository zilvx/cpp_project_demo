# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qt6 + CMake + Conan C++ desktop application with table data management, waveform charting, and HTTP backend communication. Uses the Provider pattern to abstract data sources, with conditional compilation for runtime behavior.

## Build & Development Commands

```bash
# Install dependencies (once)
conan install . --output-folder=. --build=missing -s build_type=Release

# Build entire project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build specific targets
cmake --build build --target qt_table_app          # Qt UI app
cmake --build build --target table_data_server     # HTTP backend
cmake --build build --target main                   # Console test
cmake --build build --target table_tests            # Unit tests

# Run tests
./build/tests/table_tests
# Or use CTest:
cd build && ctest

# Run individual components
open build/qt_table_app.app                        # Qt desktop app
./build/table_data_server [port]                   # HTTP server (default: 8080)
./build/main                                        # Console test program
```

**Key CMake Options** (set before running cmake):
- `USE_HTTP_DATA=ON` — Compile with HTTP JSON backend (instead of local sample data)
- `USE_PROTO_DATA=ON` — Enable Protobuf serialization for HTTP endpoints

## Architecture

### Data Source Abstraction (Provider Pattern)

The application abstracts table data fetching through `IDataProvider` interface:

```
IDataProvider (interface)
    ├── SampleDataProvider      # Local hardcoded data (default)
    ├── HttpDataProvider         # GET /api/table → JSON (USE_HTTP_DATA)
    └── HttpProtoDataProvider    # GET /api/table/proto → Protobuf (USE_PROTO_DATA)
```

All providers implement `fetchData()` which emits `dataReady(TableData)` signal → `TableWidget::loadData()` renders. Switching data sources is compile-time (CMake options), not runtime.

**File locations**:
- Interface: [src/ui/providers/IDataProvider.h](src/ui/providers/IDataProvider.h)
- Implementations: [src/ui/providers/](src/ui/providers/)
- TableWidget usage: [src/ui/widgets/TableWidget.cpp](src/ui/widgets/TableWidget.cpp#L31-L46)

### HTTP Backend Server

Built with [cpp-httplib](https://github.com/yhirose/cpp-httplib) in [src/bs/](src/bs/). Exposes three endpoints:

- `GET /api/table` — Returns table data as JSON
- `GET /api/table/proto` — Returns table data as Protobuf (only when `USE_PROTO_DATA=ON`)
- `GET /health` — Health check

The server runs standalone (not embedded in Qt app) and can be started independently: `./build/table_data_server [port]`.

### Core Components

**WaveformChart** ([src/ui/widgets/WaveformChart.cpp](src/ui/widgets/WaveformChart.cpp))
- Oscilloscope-style chart with Catmull-Rom spline interpolation → Cubic Bezier smooth curves
- "Nice Numbers" algorithm for automatic axis label spacing
- Supports sine/square/triangle waves and CSV import
- Draggable markers for data inspection
- Static utility methods: `niceStep()`, `chartRect()` — used across paintEvent/mousePress/mouseMove

**VirtualKeyboard** ([src/ui/widgets/VirtualKeyboard.cpp](src/ui/widgets/VirtualKeyboard.cpp))
- 98-key mechanical keyboard layout
- Data-driven key definitions (6 rows, lambda captures to avoid `sender()` anti-pattern)
- Replaces QTableWidget's default cell editor via `QAbstractItemDelegate`

**EditController** ([src/ui/core/EditController.cpp](src/ui/core/EditController.cpp))
- Table cell editing state machine (130 lines, previously embedded in TableWidget)
- Handles character appending, backspacing, confirming/cancelling, tab navigation
- Single responsibility: editing logic only

**PenIconDelegate** ([src/ui/core/PenIconDelegate.cpp](src/ui/core/PenIconDelegate.cpp))
- Draws pencil icon (✎) in bottom-right corner of editable cells
- Overrides `paint()` from QStyledItemDelegate

### Protobuf Schema

Define table data in [src/utils/proto/table_data.proto](src/utils/proto/table_data.proto):

```proto
message TableData {
    repeated string columns = 1;
    repeated bool   editable = 2;
    repeated Row    rows = 3;
    message Row { repeated string cells = 1; }
}
```

When `USE_PROTO_DATA=ON`, CMake generates `table_data.pb.h` and `table_data.pb.cc` in build directory at configure time (required before AUTOMOC runs).

## Testing

28 unit tests across 5 test suites using GoogleTest. Tests compile the source under test directly into `table_tests` executable:

```cmake
# tests/CMakeLists.txt
set(SRCS_UNDER_TEST
    ${CMAKE_SOURCE_DIR}/src/ui/core/WaveformData.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/EditController.cpp
    # ... other source files
)
add_executable(table_tests ${TEST_SRCS} ${SRCS_UNDER_TEST})
```

**Test suites**:
- `NiceStepTest` — Tick spacing algorithm
- `WaveformDataTest` — Waveform generation
- `WaveformChartTest` — Coordinate transforms, interpolation
- `TableDataTest` — Data structure, Protobuf round-trip
- `EditControllerTest` — Editing state machine (8 branches)

Run all: `./build/tests/table_tests`
Run via CTest: `cd build && ctest`

## Common Issues & Solutions

See [docs/troubleshooting.md](docs/troubleshooting.md) for 14 historical issues with solutions:

1. **Code Runner spdlog path** — Configure executorMap to use CMake build first
2. **Qt6 AGL framework not found** (macOS 26+) — Use stub AGL in `stub_frameworks/` with proper rpath
3. **QTableWidget sorting before data load** — Enable sorting only after `setItem()` completes
4. **QTableCornerButton white background** — Set `QTableCornerButton::section` in QSS separately
5. **PenIconDelegate positioning** — QTableWidgetItem::setIcon() places icon left of text; delegate overrides paint() for bottom-right placement

## Architecture Patterns

- **Provider Pattern** — `IDataProvider` abstracts data sources, enabling easy swapping without UI changes
- **Single Responsibility** — EditController moved out of TableWidget (210→130 lines), each component has one clear purpose
- **Compile-time Configuration** — Data source choice via CMake options (not runtime) for build consistency
- **Static Utility Methods** — WaveformChart avoids code duplication: `niceStep()`, `chartRect()` shared across paintEvent/mousePress/mouseMove
- **Lambda Captures** — VirtualKeyboard uses lambda captures instead of `sender()` for type safety
- **QSS Separation** — Styles in `.qss` files, not embedded in C++ strings
