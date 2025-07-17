#include <QuestAdbLib/QuestAdbLib.h>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "QuestAdbLib Batch Operations Example" << std::endl;
    
    // Create the ADB manager
    QuestAdbLib::QuestAdbManager manager;
    
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
        std::cout << "No devices connected. Please connect Quest devices and try again." << std::endl;
        return 1;
    }
    
    std::cout << "Found " << devicesResult.value.size() << " connected device(s)" << std::endl;
    for (const auto& device : devicesResult.value) {
        std::cout << "  - " << device.deviceId << " (" << device.model << ")" << std::endl;
    }
    
    // Set up a custom configuration
    QuestAdbLib::HeadsetConfig config;
    config.cpuLevel = 4;
    config.gpuLevel = 4;
    config.disableProximity = true;
    config.disableGuardian = false;
    config.bootTimeoutSeconds = 60;
    
    manager.setDefaultConfiguration(config);
    
    std::cout << "\\nApplying configuration to all devices..." << std::endl;
    auto configResult = manager.applyConfigurationAll(config);
    if (!configResult.success) {
        std::cerr << "Failed to apply configuration to all devices: " << configResult.error << std::endl;
    } else {
        std::cout << "Configuration applied to all devices successfully!" << std::endl;
    }
    
    // Run a shell command on all devices
    std::cout << "\\nRunning 'getprop ro.product.model' on all devices..." << std::endl;
    auto commandResult = manager.runCommandOnAll("getprop ro.product.model");
    if (commandResult.success) {
        for (const auto& [deviceId, success] : commandResult.value) {
            std::cout << "  Device " << deviceId << ": " << (success ? "Success" : "Failed") << std::endl;
        }
    }
    
    // Start metrics recording on all devices
    std::cout << "\\nStarting metrics recording on all devices for 15 seconds..." << std::endl;
    auto startMetricsResult = manager.startMetricsRecordingAll(std::chrono::seconds(15));
    if (!startMetricsResult.success) {
        std::cerr << "Failed to start metrics recording on all devices: " << startMetricsResult.error << std::endl;
    } else {
        std::cout << "Metrics recording started on all devices!" << std::endl;
    }
    
    // Wait for recording to complete
    std::this_thread::sleep_for(std::chrono::seconds(15));
    
    // Stop metrics recording on all devices
    std::cout << "\\nStopping metrics recording on all devices..." << std::endl;
    auto stopMetricsResult = manager.stopMetricsRecordingAll();
    if (!stopMetricsResult.success) {
        std::cerr << "Failed to stop metrics recording on all devices: " << stopMetricsResult.error << std::endl;
    } else {
        std::cout << "Metrics recording stopped on all devices!" << std::endl;
    }
    
    // Pull metrics from all devices
    std::cout << "\\nPulling metrics from all devices..." << std::endl;
    auto pullMetricsResult = manager.pullMetricsAll("batch_metrics");
    if (pullMetricsResult.success) {
        std::cout << "Metrics pulled from devices:" << std::endl;
        for (const auto& [deviceId, filePath] : pullMetricsResult.value) {
            if (!filePath.empty()) {
                std::cout << "  Device " << deviceId << ": " << filePath << std::endl;
            } else {
                std::cout << "  Device " << deviceId << ": Failed to pull metrics" << std::endl;
            }
        }
    }
    
    // Demonstrate individual device operations
    std::cout << "\\nDemonstrating individual device operations..." << std::endl;
    for (const auto& deviceInfo : devicesResult.value) {
        auto deviceResult = manager.getDevice(deviceInfo.deviceId);
        if (deviceResult.success) {
            auto device = deviceResult.value;
            
            std::cout << "\\nDevice " << deviceInfo.deviceId << ":" << std::endl;
            
            // Check if specific app is running
            auto appRunningResult = device->isAppRunning("com.oculus.systemux");
            if (appRunningResult.success) {
                std::cout << "  SystemUX running: " << (appRunningResult.value ? "Yes" : "No") << std::endl;
            }
            
            // Get device properties
            auto serialResult = device->getProperty("ro.serialno");
            if (serialResult.success) {
                std::cout << "  Serial: " << serialResult.value << std::endl;
            }
            
            auto osVersionResult = device->getProperty("ro.build.version.release");
            if (osVersionResult.success) {
                std::cout << "  OS Version: " << osVersionResult.value << std::endl;
            }
            
            // Check metrics files
            auto metricsFilesResult = device->getMetricsFiles();
            if (metricsFilesResult.success) {
                std::cout << "  Metrics files: " << metricsFilesResult.value.size() << std::endl;
            }
        }
    }
    
    std::cout << "\\nBatch operations example completed successfully!" << std::endl;
    return 0;
}