# plch

A port of a legacy **'90s PLC (Programmable Logic Controller) simulation program** for **Hitachi** controllers. This repository preserves the original engineering logic while migrating the application away from deprecated environments, replacing the legacy **Borland Graphics Interface (BGI)** library for modern systems using **SDL_bgi**.

## Overview

This project is a personal archive and modernization effort. The original simulation was designed in the 90s to model Hitachi PLC ladder logic, execution loops, and I/O registers. The primary goal of this port is to keep the historical software compilable and executable on modern operating systems without losing its nostalgic visual identity.

### Key Features
* **Hitachi PLC Core Simulation:** Exact replication of 90s-era Hitachi PLC register processing, scan cycles, and instruction sets.
* **Legacy Graphics Preservation:** Rendered using the classic BGI (Borland Graphics Interface) aesthetic powered by modern SDL2 hardware acceleration.
* **Modern C++ Refactor:** Cleaned up antique compiler quirks to support modern toolchains (e.g., GCC/Clang, C++17 or later).

---

## Tech Stack & Dependencies

* **Language:** C++ (Originally Turbo C++ / Borland C++)
* **Graphics Framework:** Borland Graphics Interface via **SDL_bgi** (SDL2-based compatibility layer)
* **Build System:** CMake (Orchestrated via custom shell scripts)

---

## Getting Started

### Prerequisites
To build and run this simulation, you will need to install SDL2 and the SDL_bgi development libraries.

```bash
# Ubuntu / Debian systems
sudo apt-get install libsdl2-dev libsdl2-bgi-dev
```

### Installation & Build

1. **Configure the CMake environment:**
   ```bash
   ./configure
   ```

2. **Compile the application:**
   ```bash
   ./build
   ```

### Additional Build Commands
If you need to wipe out the build artifacts and start fresh, run the cleanup script:
```bash
./clean
```

---

## Simulation Library (`libplcsim`)

The simulation engine (scan cycle, instruction set, timers, counters, special relays, external wiring and PLC link) lives in a standalone library, [`libplcsim/`](libplcsim), which the graphical simulator is built on. It has no dependency on SDL, BGI or the rest of this codebase: programs only need the public C header [`plcsim.h`](libplcsim/include/plcsim.h), which documents the full API.

```c
#include <plcsim.h>

plcsim_t *plc = plcsim_create();
if (plcsim_load_program_file(plc, "kitt.epg") != PLCSIM_OK)
    fprintf(stderr, "%s\n", plcsim_last_error(plc));
plcsim_set(plc, plcsim_find_io(plc, "KITT"), 1);
for (;;) {
    plcsim_scan(plc);
    int lamp = plcsim_get(plc, PLCSIM_OUTPUT(16));
    /* ... */
}
plcsim_destroy(plc);
```

`plcsim_dump()` prints a program's instruction list and its ladder diagram as text. The `plcsim_dump` example does it from the command line:
```bash
libplcsim/target/plcsim_dump resources/kitt.epg          # ladder diagram (same as -l, --ladder)
libplcsim/target/plcsim_dump -i resources/kitt.epg       # instruction list (--ilist)
libplcsim/target/plcsim_dump -i -l resources/kitt.epg    # both
```

Build, test and install it on its own (shared by default, `-DBUILD_SHARED_LIBS=OFF` for static):
```bash
cmake -S libplcsim -B libplcsim/target -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build libplcsim/target
ctest --test-dir libplcsim/target
cmake --install libplcsim/target
```

Then link against it with CMake (`find_package(plcsim)` + `target_link_libraries(app PRIVATE plcsim::plcsim)`) or with pkg-config (`pkg-config --cflags --libs plcsim`). The static library does not need the C++ runtime, so it links into plain C programs.

The top-level build also compiles the library; use `-DPLCH_BUILD_GUI=OFF` to skip the SDL simulator, and `-DPLCSIM_BUILD_TESTS=ON` to include the tests.

---

## Historical Context

In the 1990s, BGI was the standard graphics library for Borland Turbo C++ and Borland C++. It relied heavily on 16-bit DOS real-mode interrupts (like `int 10h`) and segmented memory architectures. 

Because modern operating systems and 64-bit CPUs cannot execute these natively, this port utilizes the excellent **SDL_bgi** library to seamlessly map classic `initgraph()`, `line()`, and `outtextxy()` graphics calls directly onto modern graphics hardware platforms using SDL2.

---

## License

Licensed under the **Apache License, Version 2.0** (the "License"). You may not use this project except in compliance with the License. You may obtain a copy of the License at:

http://apache.org

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
