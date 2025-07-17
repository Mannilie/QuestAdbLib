cmake_minimum_required(VERSION 3.16)

# Simple example
add_executable(simple_example simple_example.cpp)
target_link_libraries(simple_example QuestAdbLib)

# Device manager example
add_executable(device_manager_example device_manager_example.cpp)
target_link_libraries(device_manager_example QuestAdbLib)

# Metrics example
add_executable(metrics_example metrics_example.cpp)
target_link_libraries(metrics_example QuestAdbLib)

# Batch operations example
add_executable(batch_operations_example batch_operations_example.cpp)
target_link_libraries(batch_operations_example QuestAdbLib)

# Set output directory
set_target_properties(simple_example device_manager_example metrics_example batch_operations_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)