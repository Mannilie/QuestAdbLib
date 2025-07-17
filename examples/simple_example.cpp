#include <iostream>
#include <vector>
#include <QuestAdbLib/QuestAdbLib.h>

int main() {
    std::cout << "QuestAdbLib Simple Example" << std::endl;
    std::cout << "Version: " << QuestAdbLib::QuestAdbManager::getVersion() << std::endl;
    std::cout << "Build: " << QuestAdbLib::QuestAdbManager::getBuildInfo() << std::endl;
    
    // Create the ADB manager
    QuestAdbLib::QuestAdbManager manager;
    
    // Initialize the manager
    auto initResult = manager.initialize();
    if (!initResult.success) {
        std::cerr << "Failed to initialize ADB manager: " << initResult.error << std::endl;
        return 1;
    }
    
    std::cout << "ADB manager initialized successfully" << std::endl;
    
    // Get connected devices
    auto devicesResult = manager.getConnectedDevices();
    if (!devicesResult.success) {
        std::cerr << "Failed to get connected devices: " << devicesResult.error << std::endl;
        return 1;
    }
    
    const auto& devices = devicesResult.value;
    std::cout << "Found " << devices.size() << " connected device(s):" << std::endl;
    
    for (const auto& device : devices) {
        std::cout << "  Device ID: " << device.deviceId << std::endl;
        std::cout << "  Status: " << device.status << std::endl;
        std::cout << "  Model: " << device.model << std::endl;
        std::cout << "  Battery: " << device.batteryLevel << "%" << std::endl;
        std::cout << "  Running Apps: " << device.runningApps.size() << std::endl;
        std::cout << std::endl;
    }
    
    if (!devices.empty()) {
        // Get the first device and perform some operations
        auto deviceResult = manager.getDevice(devices[0].deviceId);
        if (deviceResult.success) {
            auto device = deviceResult.value;
            
            // Get device model
            auto modelResult = device->getModel();
            if (modelResult.success) {
                std::cout << "Device model: " << modelResult.value << std::endl;
            }
            
            // Get battery level
            auto batteryResult = device->getBatteryLevel();
            if (batteryResult.success) {
                std::cout << "Battery level: " << batteryResult.value << "%" << std::endl;
            }
            
            // Get running apps
            auto appsResult = device->getRunningApps();
            if (appsResult.success) {
                std::cout << "Running apps:" << std::endl;
                for (const auto& app : appsResult.value) {
                    std::cout << "  - " << app << std::endl;
                }
            }
        }
    }
    
    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}
