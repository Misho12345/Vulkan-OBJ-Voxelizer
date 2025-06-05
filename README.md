# Vulkan-OBJ-Voxelizer

A Vulkan-based tool that converts an `.obj` file into a 3D texture representing the voxelized version of the object.

## Features

- Converts 3D models in OBJ format to voxel grids.
- Outputs the voxelized data as a 3D texture.
- Utilizes Vulkan for high-performance rendering and processing.
- Flexible configuration for voxel resolution and model input.

## Getting Started

### Prerequisites

- C++17 compatible compiler
- Vulkan SDK (https://vulkan.lunarg.com/)
- CMake 3.10+
- Git
- (Optional) Ninja build system

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

### Running

After building, you can run the voxelizer tool.  
Example usage:

```bash
./Vulkan-OBJ-Voxelizer --input path/to/model.obj --output output_texture.dat --resolution 128
```

## Project Structure

- `src/` — Main source code
- `shaders/` — Vulkan shader programs
- `CMakeLists.txt` — Build configuration
- `README.md` — Project documentation

## Contributing

Contributions, issues, and feature requests are welcome!  
Feel free to open issues or submit pull requests.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
