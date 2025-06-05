# Vulkan-OBJ-Voxelizer

A Vulkan-based tool that converts a 3D model in `.obj` format into a 3D texture representing the voxelized version of the object. This tool leverages Vulkan's high-performance compute and graphics capabilities for efficient, flexible voxelization.

## Table of Contents
- [Features](#features)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Building](#building)
  - [Running](#running)
- [Project Structure](#project-structure)
- [Customization](#customization)
- [Troubleshooting](#troubleshooting)

---

## Features

- Converts 3D models in OBJ format to voxel grids of configurable resolution.
- Generates output as a raw 3D texture suitable for visualization or further processing.
- Utilizes Vulkan for fast, parallelized processing.
- Customizable voxel resolution and input/output paths.
- Modular code structure for easy extension.


---

## Getting Started

### Prerequisites

- C++17 compatible compiler (e.g., GCC 8+, MSVC 2019+)
- Vulkan SDK ([Download](https://vulkan.lunarg.com/))
- CMake 3.10 or newer
- Git
- (Optional) Ninja build system for faster builds

### Building

Clone the repository:
```bash
git clone https://github.com/Misho12345/Vulkan-OBJ-Voxelizer.git
cd Vulkan-OBJ-Voxelizer
```

Create a build directory and configure the project:
```bash
mkdir build
cd build
cmake ..
make
```
(Or use `cmake -G Ninja .. && ninja` if you have Ninja installed.)

### Running

After building, run the voxelizer tool from the build directory.

Usage:
```bash
./Vulkan-OBJ-Voxelizer
```

## Project Structure

- `src/` — Main source code, including Vulkan initialization, OBJ parsing, and voxelization logic
- `shaders/` — Vulkan shader programs for GPU-accelerated processing
- `CMakeLists.txt` — Build configuration script
- `README.md` — This documentation
- `LICENSE` — Project license

---

## Customization

- **Voxel Resolution:** Change the resolution parameter at runtime to control the level of detail.
- **Shader Code:** Modify shaders in `shaders/` to experiment with different voxelization techniques or add features (like color or normal storage).
- **Code Extensions:** The modular C++ codebase makes it easy to add support for more file formats or output types.

---

## Troubleshooting

- **Vulkan Errors:** Ensure your graphics drivers and Vulkan SDK are up to date.
- **Build Issues:** Verify CMake version, and that all dependencies are installed. Use `cmake --version` and `vulkaninfo` for diagnostics.
- **OBJ Loading:** Check that your `.obj` file is not corrupted and is in standard format.

---
