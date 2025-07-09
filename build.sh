#!/bin/bash

# Let's Code build script
# Usage: ./build.sh [debug|release|clean|windows|install|test]

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "Checking system dependencies..."
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed. Please install CMake first."
        exit 1
    fi
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        print_error "G++ compiler is not installed. Please install build-essential."
        exit 1
    fi
    
    # 检查FLTK
    if ! pkg-config --exists fltk; then
        print_warning "FLTK development library not found. Please install libfltk1.3-dev."
        print_info "Ubuntu/Debian: sudo apt install libfltk1.3-dev"
        print_info "CentOS/RHEL: sudo yum install fltk-devel"
    fi
    
    print_success "Dependency check complete."
}

# 清理构建目录
clean_build() {
    print_info "Cleaning build directories..."
    rm -rf build build-debug build-win
    print_success "Clean complete."
}

# 构建项目
build_project() {
    local build_type=$1
    local build_dir=$2
    local cmake_options=$3
    
    print_info "Building project (${build_type})..."
    
    # 创建构建目录
    mkdir -p $build_dir
    cd $build_dir
    
    # 配置项目
    print_info "Configuring CMake..."
    cmake .. $cmake_options
    
    # 编译项目
    print_info "Compiling..."
    make -j$(nproc)
    
    cd ..
    print_success "Build complete! Executable is in: $build_dir/bin/lets_code"
}

# 主函数
main() {
    local action=${1:-release}
    
    case $action in
        "debug")
            check_dependencies
            build_project "Debug" "build-debug" "-DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON"
            ;;
        "release")
            check_dependencies
            build_project "Release" "build" "-DCMAKE_BUILD_TYPE=Release"
            ;;
        "clean")
            clean_build
            ;;
        "windows")
            check_dependencies
            print_info "Starting Windows cross-compilation..."
            if [ ! -f "mingw-toolchain.cmake" ]; then
                print_error "mingw-toolchain.cmake file not found."
                exit 1
            fi
            build_project "Windows" "build-win" "-DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS='-static-libgcc -static-libstdc++'"
            ;;
        "install")
            check_dependencies
            build_project "Release" "build" "-DCMAKE_BUILD_TYPE=Release"
            cd build
            sudo make install
            cd ..
            print_success "Install complete!"
            ;;
        "test")
            check_dependencies
            build_project "Debug" "build-debug" "-DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON -DBUILD_TESTS=ON"
            cd build-debug
            make test
            cd ..
            ;;
        *)
            echo "Usage: $0 [debug|release|clean|windows|install|test]"
            echo ""
            echo "Options:"
            echo "  debug     - Build in debug mode"
            echo "  release   - Build in release mode (default)"
            echo "  clean     - Clean build directories"
            echo "  windows   - Cross-compile for Windows"
            echo "  install   - Build and install"
            echo "  test      - Build and run tests"
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@" 