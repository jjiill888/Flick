# Let's Code - Lightweight Code Editor

A cross-platform lightweight code editor based on FLTK, featuring a modern interface and rich functionality.

## Features

- 🎨 **Modern UI** - Supports dark and light themes
- 📁 **File Tree Browsing** - Intuitive file and folder management
- 🔍 **Smart Search** - Find, replace, and global search support
- 💾 **Auto Save** - Smart file state management
- ⌨️ **Keyboard Shortcuts** - Boost coding efficiency
- 🌍 **Cross-Platform** - Supports Linux, Windows, and macOS
- ⚡ **Lightweight** - Fast startup, low resource usage

## System Requirements

- **Linux**: FLTK 1.4.3+, CMake 3.15+, GCC 7+
- **Windows**: MinGW-w64, CMake 3.15+
- **macOS**: Xcode Command Line Tools, CMake 3.15+

## Install Dependencies

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libfltk1.3-dev
```

### CentOS/RHEL/Fedora
```bash
sudo yum install gcc-c++ cmake fltk-devel
# Or use dnf (Fedora)
sudo dnf install gcc-c++ cmake fltk-devel
```

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-fltk
```

## Build & Install

### Native Linux Build

```bash
# Clone the project
git clone https://github.com/your-username/lets-code.git
cd lets-code

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run the program
./bin/lets_code
```

### Debug Build

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON
make -j$(nproc)
```

### Windows Cross-Compile (from Linux)

```bash
# Install MinGW toolchain
sudo apt install mingw-w64 cmake g++-mingw-w64

# Create Windows build directory
mkdir build-win && cd build-win

# Configure cross-compilation
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"

# Build
make -j$(nproc)
```

After building, the Windows executable `lets_code.exe` will be in the `build-win/bin/` directory.

## Usage

### Basic Operations

| Feature        | Shortcut         | Description                        |
|---------------|------------------|------------------------------------|
| New File      | Ctrl+N           | Create a new text file             |
| Open File     | Ctrl+O           | Open an existing file              |
| Save File     | Ctrl+S           | Save the current file              |
| Find          | Ctrl+F           | Find in the current file           |
| Replace       | Ctrl+H           | Replace in the current file        |
| Global Search | Ctrl+Shift+F     | Search in the entire project       |
| Switch Theme  | Menu Bar         | Toggle between dark/light themes   |

### File Tree Operations

- **Right-click Menu**: Access context menu by right-clicking on the file tree
- **New File/Folder**: Create via right-click menu
- **Delete**: Supports deleting files and folders
- **Refresh**: Update the file tree display

### Editor Features

- **Syntax Highlighting**: Supports C/C++, Python, JavaScript, etc.
- **Line Numbers**: Automatically display and adjust line number width
- **Font Zoom**: Ctrl + mouse wheel to adjust font size
- **Word Wrap**: Supports automatic line wrapping for long lines

## Configuration

The program creates a `.lets_code` config folder in the user's home directory, saving the following settings:

- `font_size`: Font size
- `theme`: Theme setting (0=dark, 1=light)
- `tree_width`: File tree width
- `last_file`: Last opened file
- `last_folder`: Last opened folder

## Development

### Project Structure

```
src/
├── main.cpp              # Program entry
├── editor_window.hpp/cpp # Main window class
├── editor_state.hpp/cpp  # State management
├── file_tree.hpp/cpp     # File tree component
├── utils.hpp/cpp         # Utility functions
├── SearchReplace.hpp/cpp # Search and replace features
└── scrollbar_theme.hpp/cpp # Scrollbar theme
```

### Adding New Features

1. Declare new functions in the relevant header file
2. Implement the feature in the corresponding cpp file
3. Update CMakeLists.txt (if new files are added)
4. Test the feature and update documentation

### Debugging

After building in debug mode, the program outputs detailed debug information:

```bash
./bin/lets_code --debug
```

## Contributing

Issues and Pull Requests are welcome!

### Development Environment Setup

1. Fork the project
2. Create a feature branch: `git checkout -b feature/new-feature`
3. Commit your changes: `git commit -am 'Add new feature'`
4. Push the branch: `git push origin feature/new-feature`
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Changelog

### v1.0.0
- Initial release
- Basic code editing features
- File tree browsing
- Theme support
- Search and replace features

## Issue Reporting

If you encounter problems or have suggestions:

1. Check [Issues](https://github.com/your-username/lets-code/issues)
2. Create a new Issue describing the problem
3. Provide system information and error logs

---

**Let's Code** - Make coding easier!