#!/bin/bash

# Let's Code 构建脚本
# 用法: ./build.sh [debug|release|clean]

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
    print_info "检查系统依赖..."
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        print_error "G++ 编译器未安装，请先安装 build-essential"
        exit 1
    fi
    
    # 检查FLTK
    if ! pkg-config --exists fltk; then
        print_warning "FLTK 开发库未找到，请安装 libfltk1.3-dev"
        print_info "Ubuntu/Debian: sudo apt install libfltk1.3-dev"
        print_info "CentOS/RHEL: sudo yum install fltk-devel"
    fi
    
    print_success "依赖检查完成"
}

# 清理构建目录
clean_build() {
    print_info "清理构建目录..."
    rm -rf build build-debug build-win
    print_success "清理完成"
}

# 构建项目
build_project() {
    local build_type=$1
    local build_dir=$2
    local cmake_options=$3
    
    print_info "开始构建项目 (${build_type})..."
    
    # 创建构建目录
    mkdir -p $build_dir
    cd $build_dir
    
    # 配置项目
    print_info "配置 CMake..."
    cmake .. $cmake_options
    
    # 编译项目
    print_info "编译项目..."
    make -j$(nproc)
    
    cd ..
    print_success "构建完成！可执行文件位于: $build_dir/bin/lets_code"
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
            print_info "开始Windows交叉编译..."
            if [ ! -f "mingw-toolchain.cmake" ]; then
                print_error "未找到 mingw-toolchain.cmake 文件"
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
            print_success "安装完成！"
            ;;
        "test")
            check_dependencies
            build_project "Debug" "build-debug" "-DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON -DBUILD_TESTS=ON"
            cd build-debug
            make test
            cd ..
            ;;
        *)
            echo "用法: $0 [debug|release|clean|windows|install|test]"
            echo ""
            echo "选项:"
            echo "  debug     - 调试模式构建"
            echo "  release   - 发布模式构建 (默认)"
            echo "  clean     - 清理构建目录"
            echo "  windows   - Windows交叉编译"
            echo "  install   - 构建并安装"
            echo "  test      - 构建并运行测试"
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@" 