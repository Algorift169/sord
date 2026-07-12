#!/usr/bin/env bash

# deps.sh - Install Sord compiler and FTXUI library dependencies across Linux distributions

set -e

echo "=== Sord Dependency Installer ==="

# Helper to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Detect package manager and install system compiler/build tools
if command_exists apt-get; then
    echo "Detected Debian/Ubuntu-based system."
    sudo apt-get update
    sudo apt-get install -y build-essential pkg-config cmake git
    
    # Try installing packaged ftxui if available
    echo "Checking for ftxui packages..."
    if apt-cache show libftxui-dev >/dev/null 2>&1; then
        sudo apt-get install -y libftxui-dev
    else
        echo "libftxui-dev not found in package repositories. Will build from source..."
        BUILD_FTXUI_FROM_SOURCE=true
    fi

elif command_exists dnf; then
    echo "Detected RHEL/Fedora-based system."
    sudo dnf groupinstall -y "Development Tools"
    sudo dnf install -y gcc-c++ cmake pkgconfig git
    BUILD_FTXUI_FROM_SOURCE=true

elif command_exists pacman; then
    echo "Detected Arch-based system."
    sudo pacman -Syu --noconfirm base-devel cmake git
    
    # Try installing ftxui from Arch repository if available
    if pacman -Si ftxui >/dev/null 2>&1; then
        sudo pacman -S --noconfirm ftxui
    else
        BUILD_FTXUI_FROM_SOURCE=true
    fi

elif command_exists zypper; then
    echo "Detected openSUSE-based system."
    sudo zypper install -y -t pattern devel_C_C++
    sudo zypper install -y cmake pkg-config git
    BUILD_FTXUI_FROM_SOURCE=true

else
    echo "Unsupported or unrecognized distribution. Please install C++20 build tools, cmake, pkg-config, and ftxui manually."
    exit 1
fi

# Build FTXUI from source if not package-installed
if [ "$BUILD_FTXUI_FROM_SOURCE" = true ]; then
    echo "=== Building and Installing FTXUI from source ==="
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    git clone https://github.com/ArthurSonzogni/FTXUI.git
    cd FTXUI
    mkdir build
    cd build
    cmake .. -DFTXUI_BUILD_EXAMPLES=OFF -DFTXUI_BUILD_TESTS=OFF
    make
    sudo make install
    # Clean up
    cd /
    rm -rf "$TEMP_DIR"
    echo "FTXUI successfully built and installed."
fi

echo "=== All dependencies installed successfully! ==="
