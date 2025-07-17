#include <QuestAdbLib/QuestAdbLib.h>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "QuestAdbLib Device Manager Example" << std::endl;
    
    // Create the ADB manager
    QuestAdbLib::QuestAdbManager manager;
    
    // Set up callbacks
    manager.setDeviceStatusCallback([](const std::string& deviceId, const std::string& status) {
        std::cout << "Device " << deviceId << " status changed to: " << status << std::endl;
    });
    
    manager.setDeviceListCallback([](const std::vector<QuestAdbLib::DeviceInfo>& devices) {
        std::cout << "Device list updated - " << devices.size() << " device(s) connected" << std::endl;
        for (const auto& device : devices) {
            std::cout << "  - " << device.deviceId << " (" << device.status << ")" << std::endl;
        }
    });
    
    // Initialize the manager
    auto initResult = manager.initialize();
    if (!initResult.success) {
        std::cerr << "Failed to initialize ADB manager: " << initResult.error << std::endl;
        return 1;
    }
    
    std::cout << "ADB manager initialized successfully" << std::endl;
    
    // Start device monitoring
    auto monitorResult = manager.startDeviceMonitoring(3); // Check every 3 seconds
    if (!monitorResult.success) {
        std::cerr << "Failed to start device monitoring: " << monitorResult.error << std::endl;
        return 1;
    }
    
    std::cout << "Device monitoring started. Monitoring for 30 seconds..." << std::endl;
    std::cout << "Connect or disconnect devices to see real-time updates." << std::endl;
    
    // Monitor for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // Stop monitoring
    manager.stopDeviceMonitoring();
    std::cout << "Device monitoring stopped." << std::endl;
    
    // Get final device list
    auto devicesResult = manager.getConnectedDevices();
    if (devicesResult.success) {
        std::cout << "Final device list:" << std::endl;
        for (const auto& device : devicesResult.value) {
            std::cout << "  Device: " << device.deviceId << std::endl;
            std::cout << "    Status: " << device.status << std::endl;
            std::cout << "    Model: " << device.model << std::endl;
            std::cout << "    Battery: " << device.batteryLevel << "%" << std::endl;
            std::cout << "    Apps: " << device.runningApps.size() << std::endl;
        }
    }
    
    std::cout << "Device manager example completed!" << std::endl;
    return 0;
}