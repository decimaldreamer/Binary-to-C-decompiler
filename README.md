# Binary-to-C Decompiler

A powerful binary-to-C decompiler that supports multiple binary formats and provides advanced type inference capabilities.

## Features

- **Multiple Binary Format Support**
  - ELF (Executable and Linkable Format)
  - PE (Portable Executable)
  - Mach-O (Mach Object)
  - Support for segments, labels, and relocations

- **Advanced Type Inference**
  - Basic type detection (char, short, int, long long)
  - Array detection
  - Pointer detection
  - Structure and union detection
  - Enum detection
  - Automatic C code generation for detected types

- **Performance Optimizations**
  - Caching mechanism for frequently accessed data
  - Parallel processing support (OpenMP)
  - Memory optimizations
  - Dynamic array management

- **Modern GUI Interface**
  - Qt6-based interface
  - File open/save support
  - Progress bar
  - Status bar
  - Menu and toolbar

## Building

### Prerequisites

- CMake 3.10 or higher
- C11 compatible compiler
- C++17 compatible compiler
- Qt6
- OpenMP

### Build Steps

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

### Command Line Interface

```bash
./b2c_decompiler input_file output_file.c
```

### Graphical User Interface

```bash
./b2c_gui
```

## Project Structure

- `main.c` - Main entry point
- `opsoup.h` - Core header file
- `ref.c` - Reference handling
- `disasm.c` - Disassembly
- `elf.c` - ELF format support
- `pe.c` - PE format support
- `macho.c` - Mach-O format support
- `image.c` - Image handling
- `label.c` - Label management
- `data.c` - Data analysis
- `optimize.c` - Performance optimizations
- `analysis.c` - Type analysis
- `gui/` - GUI components
  - `mainwindow.cpp` - Main window implementation
  - `mainwindow.h` - Main window header

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

