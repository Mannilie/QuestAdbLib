#include <QuestAdbLib/QuestAdbLib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

int main() {
    std::cout << "QuestAdbLib Metrics Example" << std::endl;
    
    // Create the ADB manager
    QuestAdbLib::QuestAdbManager manager;
    
    // Set up metrics progress callback
    manager.setMetricsProgressCallback([](const std::string& deviceId, double progress) {
        std::cout << "Device " << deviceId << " metrics progress: " << progress << "%" << std::endl;
    });
    
    // Initialize the manager
    auto initResult = manager.initialize();
    if (!initResult.success) {
        std::cerr << "Failed to initialize ADB manager: " << initResult.error << std::endl;
        return 1;
    }
    
    // Get connected devices
    auto devicesResult = manager.getConnectedDevices();
    if (!devicesResult.success) {
        std::cerr << "Failed to get connected devices: " << devicesResult.error << std::endl;
        return 1;
    }
    
    if (devicesResult.value.empty()) {
        std::cout << "No devices connected. Please connect a Quest device and try again." << std::endl;
        return 1;
    }
    
    const auto& device = devicesResult.value[0];
    std::cout << "Using device: " << device.deviceId << " (" << device.model << ")" << std::endl;
    
    // Get device handle
    auto deviceResult = manager.getDevice(device.deviceId);
    if (!deviceResult.success) {
        std::cerr << "Failed to get device handle: " << deviceResult.error << std::endl;
        return 1;
    }
    
    auto deviceHandle = deviceResult.value;
    
    // Apply basic configuration
    QuestAdbLib::HeadsetConfig config;
    config.cpuLevel = 4;
    config.gpuLevel = 4;
    config.disableProximity = true;
    
    std::cout << "Applying device configuration..." << std::endl;
    auto configResult = deviceHandle->applyConfiguration(config);
    if (!configResult.success) {
        std::cerr << "Failed to apply configuration: " << configResult.error << std::endl;
        return 1;
    }
    
    std::cout << "Configuration applied successfully!" << std::endl;
    
    // Start metrics recording
    std::cout << "Starting metrics recording for 10 seconds..." << std::endl;
    auto startResult = deviceHandle->startMetricsRecording();
    if (!startResult.success) {
        std::cerr << "Failed to start metrics recording: " << startResult.error << std::endl;
        return 1;
    }
    
    std::cout << "Metrics recording started. Please interact with the headset..." << std::endl;
    
    // Wait for recording to complete
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Stop metrics recording
    std::cout << "Stopping metrics recording..." << std::endl;
    auto stopResult = deviceHandle->stopMetricsRecording();
    if (!stopResult.success) {
        std::cerr << "Failed to stop metrics recording: " << stopResult.error << std::endl;
        return 1;
    }
    
    std::cout << "Metrics recording stopped." << std::endl;
    
    // Create metrics directory
    std::string metricsDir = "metrics";
    std::filesystem::create_directories(metricsDir);
    
    // Pull metrics
    std::cout << "Pulling metrics from device..." << std::endl;
    auto pullResult = deviceHandle->pullLatestMetrics(metricsDir);
    if (!pullResult.success) {
        std::cerr << "Failed to pull metrics: " << pullResult.error << std::endl;
        return 1;
    }
    
    std::cout << "Metrics saved to: " << pullResult.value << std::endl;
    
    // Check file size
    try {
        auto fileSize = std::filesystem::file_size(pullResult.value);
        std::cout << "Metrics file size: " << fileSize << " bytes" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error checking file size: " << e.what() << std::endl;
    }
    
    std::cout << "Metrics example completed successfully!" << std::endl;
    return 0;
}