# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(CacheSimulator VERSION 1.0)

# Set the C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# Add the executable target
add_executable(cache_simulator sim.cpp)

# Optionally, add a "clean" target (CMake handles this automatically)
# You can run `make clean` or `cmake --build . --target clean`

# Optionally, add a "rebuild" target
add_custom_target(rebuild
    COMMAND ${CMAKE_COMMAND} --build . --clean-first
    COMMENT "Rebuilding the project"
)
