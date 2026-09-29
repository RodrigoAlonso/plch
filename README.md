# [Project Name]

A modern C++ port of a legacy **1990s PLC (Programmable Logic Controller) simulation program**. This repository preserves the original engineering logic while migrating the application away from deprecated environments, replacing or wrapper-housing the legacy **Borland Graphics Interface (BGI)** library for modern systems.

## 🚀 Overview

This project is a personal archive and modernization effort. The original simulation was designed in the 1990s to model PLC ladder logic, execution loops, and I/O registers. The primary goal of this port is to keep the historical software compilable and executable on modern operating systems without losing its nostalgic visual identity.

### Key Features
* **PLC Core Simulation:** Exact replication of 90s-era PLC register processing, scan cycles, and instruction sets.
* **Legacy Graphics Preservation:** Rendered using the classic BGI (Borland Graphics Interface) aesthetic.
* **Modern C++ Refactor:** Cleaned up antique compiler quirks to support modern toolchains (e.g., GCC/Clang, C++17 or later).

---

## 🛠️ Tech Stack & Dependencies

* **Language:** C++ (Originally Turbo C++ / Borland C++)
* **Graphics Framework:** Borland Graphics Interface (BGI) 
  * *Note: Ported via [Specify your wrapper here, e.g., WinBGIm, SDL-bgi, or Libgraph]*
* **Build System:** [e.g., CMake / Make / Visual Studio]

---

## ⚡ Getting Started

### Prerequisites
To build and run this simulation, you will need a compiler capable of handling the BGI compatibility layer. 

```bash
# Example for installing dependencies (adjust for your specific BGI port)
sudo apt-get install libsdl2-dev  # If using SDL-bgi
```

### Installation & Build

1. **Clone the repository:**
   ```bash
   git clone https://github.com
   cd your-repo-name
   ```

2. **Build the project:**
   ```bash
   # Replace with your actual build commands
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run the simulation:**
   ```bash
   ./plc_simulator
   ```

---

## 📜 Historical Context

In the 1990s, BGI was the standard graphics library for Borland Turbo C++ and Borland C++. It relied heavily on 16-bit DOS real-mode interrupts (like `int 10h`) and segmented memory architectures. 

Because modern operating systems and 64-bit CPUs cannot execute these natively, this port utilizes a compatibility wrapper to map the classic `initgraph()`, `line()`, and `outtextxy()` calls onto modern graphics hardware APIs.

---

## 🧑�💻 License

This project is licensed under the [MIT License](LICENSE) - see the file for details. Original software artifacts remain the property of their respective historical creators.
