# QuestAdbLib

A cross-platform C++ library for managing Meta Quest VR headsets through ADB (Android Debug Bridge). This library provides a high-level interface for device management, configuration, and metrics collection.

## Features

- 🔍 **Device Discovery**: Automatically detect and manage connected Quest devices
- ⚙️ **Configuration Management**: Apply VR-specific settings (CPU/GPU levels, proximity, guardian)
- 📊 **Metrics Collection**: Record and collect OVR performance metrics
- 🔄 **Batch Operations**: Perform operations on multiple devices simultaneously
- 🌐 **Cross-Platform**: Works on Windows, macOS, and Linux
- 📦 **Easy Integration**: CMake-based build system with clean C++ API

## Quick Start

### Prerequisites

- CMake 3.16 or later
- C++17 compatible compiler
- Android Debug Bridge (ADB) installed (see ADB Installation section below)

### Building

**Windows:**
```bash
./build.bat
```

**Unix/Linux/macOS:**
```bash
./build.sh
```

**Manual CMake Build:**
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON ..
cmake --build . --config Release
```

### Basic Usage

```cpp
#include <QuestAdbLib/QuestAdbLib.h>

int main() {
    // Create and initialize the ADB manager
    QuestAdbLib::QuestAdbManager manager;
    auto initResult = manager.initialize();
    
    if (!initResult.success) {
        std::cerr << "Failed to initialize: " << initResult.error << std::endl;
        return 1;
    }
    
    // Get connected devices
    auto devicesResult = manager.getConnectedDevices();
    if (devicesResult.success) {
        for (const auto& device : devicesResult.value) {
            std::cout << "Device: " << device.deviceId 
                      << " (" << device.model << ")" << std::endl;
        }
    }
    
    return 0;
}
```

## API Overview

### Core Classes

- **`QuestAdbManager`**: Main manager class for device operations
- **`AdbDevice`**: Represents a single Quest device with all operations
- **`AdbCommand`**: Low-level ADB command execution
- **`DeviceInfo`**: Device information structure
- **`HeadsetConfig`**: VR headset configuration parameters

### Key Operations

#### Device Management
```cpp
// Get all connected devices
auto devices = manager.getConnectedDevices();

// Get specific device
auto device = manager.getDevice("device_id");

// Monitor device changes
manager.setDeviceStatusCallback([](const std::string& deviceId, const std::string& status) {
    std::cout << "Device " << deviceId << " status: " << status << std::endl;
});
manager.startDeviceMonitoring();
```

#### Configuration
```cpp
// Create configuration
QuestAdbLib::HeadsetConfig config;
config.cpuLevel = 4;
config.gpuLevel = 4;
config.disableProximity = true;

// Apply to single device
auto device = manager.getDevice("device_id");
device->applyConfiguration(config);

// Apply to all devices
manager.applyConfigurationAll(config);
```

#### Metrics Collection
```cpp
// Start recording on device
auto device = manager.getDevice("device_id");
device->startMetricsRecording();

// Wait for recording...
std::this_thread::sleep_for(std::chrono::seconds(30));

// Stop and pull metrics
device->stopMetricsRecording();
auto metricsPath = device->pullLatestMetrics("./metrics");
```

#### Batch Operations
```cpp
// Reboot all devices
manager.rebootAndWaitAll();

// Run command on all devices
auto results = manager.runCommandOnAll("getprop ro.product.model");

// Start metrics on all devices
manager.startMetricsRecordingAll(std::chrono::seconds(30));
```

## Examples

The library comes with several example programs:

- **`simple_example`**: Basic device detection and information
- **`device_manager_example`**: Real-time device monitoring
- **`metrics_example`**: Metrics collection workflow
- **`batch_operations_example`**: Multi-device operations

Build and run examples:
```bash
mkdir build && cd build
cmake -DBUILD_EXAMPLES=ON ..
make
./examples/simple_example
```

## Configuration Options

### HeadsetConfig Parameters

| Parameter | Description | Range | Default |
|-----------|-------------|-------|---------|
| `cpuLevel` | CPU performance level | 0-4 | 4 |
| `gpuLevel` | GPU performance level | 0-4 | 4 |
| `disableProximity` | Disable proximity sensor | bool | true |
| `disableGuardian` | Disable Guardian boundary | bool | false |
| `bootTimeoutSeconds` | Device boot timeout | int | 60 |
| `waitTimeoutSeconds` | Command wait timeout | int | 15 |
| `testDuration` | Metrics recording duration | seconds | 30 |

### Device Status Values

- `connected`: Device is connected and authorized
- `unauthorized`: Device connected but not authorized
- `disconnected`: Device is not connected
- `rebooting`: Device is currently rebooting
- `configuring`: Configuration is being applied
- `ready`: Device is configured and ready
- `error`: Device encountered an error

## CMake Integration

To use QuestAdbLib in your CMake project:

```cmake
find_package(QuestAdbLib REQUIRED)
target_link_libraries(your_target QuestAdbLib::QuestAdbLib)
```

Or as a subdirectory:
```cmake
add_subdirectory(QuestAdbLib)
target_link_libraries(your_target QuestAdbLib)
```

## Platform Support

- **Windows**: Tested with Visual Studio 2019+ and MinGW
- **macOS**: Tested with Xcode and command-line tools
- **Linux**: Tested with GCC and Clang

## ADB Installation

QuestAdbLib requires Android Debug Bridge (ADB) to be installed on your system. Choose one of these installation methods:

### Option 1: Android SDK Platform Tools (Recommended)

Download the official Android SDK Platform Tools from Google:
- **Windows**: https://dl.google.com/android/repository/platform-tools-latest-windows.zip
- **macOS**: https://dl.google.com/android/repository/platform-tools-latest-darwin.zip
- **Linux**: https://dl.google.com/android/repository/platform-tools-latest-linux.zip

Extract and add the `platform-tools` directory to your system PATH.

### Option 2: Package Managers

**Windows (Chocolatey):**
```bash
choco install adb
```

**macOS (Homebrew):**
```bash
brew install android-platform-tools
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install adb
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install android-tools
```

### Option 3: Android Studio

If you have Android Studio installed, ADB is included in the SDK. Add the SDK's `platform-tools` directory to your PATH:
- Windows: `%LOCALAPPDATA%\Android\Sdk\platform-tools`
- macOS/Linux: `~/Android/Sdk/platform-tools`

### Custom ADB Path

You can specify a custom ADB path in your code:
```cpp
QuestAdbLib::QuestAdbManager manager("/path/to/adb");
```

### ADB Path Detection

The library automatically searches for ADB in these locations:
- Next to the executable
- System PATH
- Common Android SDK installation directories
- Current working directory

## Error Handling

All operations return `Result<T>` objects with success/error information:

```cpp
auto result = device->getModel();
if (result.success) {
    std::cout << "Model: " << result.value << std::endl;
} else {
    std::cerr << "Error: " << result.error << std::endl;
}
```

## Thread Safety

- `QuestAdbManager` is thread-safe for concurrent operations
- Individual `AdbDevice` instances are not thread-safe
- Callbacks are executed on the monitoring thread

## License

MIT License - see LICENSE file for details.

## CI/CD Pipeline

This project uses GitHub Actions for continuous integration and deployment:

- **Multi-platform builds**: Automatically builds on Windows, macOS, and Linux
- **Debug and Release**: Both build configurations are tested
- **Code quality checks**: Static analysis with clang-tidy and format checking
- **Automated releases**: Creates release artifacts for all platforms
- **Example verification**: Ensures all examples compile and run successfully

The CI pipeline runs on every push and pull request to main and develop branches.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with tests
4. Ensure code follows formatting standards (`clang-format`)
5. Submit a pull request

The CI pipeline will automatically build and test your changes across all platforms.

## Troubleshooting

### Common Issues

**ADB not found**: Follow the ADB Installation section above to install ADB properly.

**Device not authorized**: Enable USB debugging on the headset and authorize the computer.

**Metrics collection fails**: Ensure the OVR Monitor Metrics Service is running on the headset.

**Build errors**: Check that you have C++17 support and all dependencies installed.

### Debug Mode

Enable debug output by setting the log level:
```cpp
// Set environment variable or use debug build
std::cout << "Using ADB path: " << QuestAdbLib::QuestAdbManager::findAdbPath() << std::endl;
```