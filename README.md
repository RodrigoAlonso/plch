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

## Historical Context

In the 1990s, BGI was the standard graphics library for Borland Turbo C++ and Borland C++. It relied heavily on 16-bit DOS real-mode interrupts (like `int 10h`) and segmented memory architectures. 

Because modern operating systems and 64-bit CPUs cannot execute these natively, this port utilizes the excellent **SDL_bgi** library to seamlessly map classic `initgraph()`, `line()`, and `outtextxy()` graphics calls directly onto modern graphics hardware platforms using SDL2.

---

## License

Licensed under the **Apache License, Version 2.0** (the "License"). You may not use this project except in compliance with the License. You may obtain a copy of the License at:

http://apache.org

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
