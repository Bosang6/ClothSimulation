#!/bin/bash

# set path
PROJECT_DIR=$(pwd)
BUILD_DIR="$PROJECT_DIR/build"

# check build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "Create build..."
    mkdir -p "$BUILD_DIR"
fi

# enter build 
cd "$BUILD_DIR"

# CMake
cmake ..
cmake --build .

# Return to the project root directory
cd "$PROJECT_DIR"

# Run the executable file
EXECUTABLE="$PROJECT_DIR/ClothSimulation"
if [ -f "$EXECUTABLE" ]; then
    ./"ClothSimulation"
else
    echo "ERROE: DOSE NOT ESIST EXECUTABLE FILE!"
fi
