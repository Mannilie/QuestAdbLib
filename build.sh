#!/bin/bash

echo "Building QuestAdbLib for Unix/Linux..."

# Create build directory
mkdir -p build
cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed"
    exit 1
fi

# Build
cmake --build . --config Release -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo "Examples can be found in: build/examples/"
echo "Library files can be found in: build/"

# Make examples executable
chmod +x examples/*_example 2>/dev/null || true

echo ""
echo "To run examples:"
echo "  cd build"
echo "  ./examples/simple_example"
echo "  ./examples/device_manager_example"
echo "  ./examples/metrics_example"
echo "  ./examples/batch_operations_example"