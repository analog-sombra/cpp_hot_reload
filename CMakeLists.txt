cmake_minimum_required(VERSION 3.28)

# vcpkg integration
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project(CPP_HOT_RELOAD VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)

# Source files for main executable (static linked)
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# -------------------------------------------------------------------------------

# ---- DLL PLUGIN BUILD (hot-reloadable) ----
file(GLOB_RECURSE PLUGIN_SOURCES "${CMAKE_SOURCE_DIR}/hr_src/*.cpp")

add_library(plug SHARED ${PLUGIN_SOURCES})
target_include_directories(plug PRIVATE ${CMAKE_SOURCE_DIR}/hr_include)

# Link SFML to the plugin DLL
target_link_libraries(plug PRIVATE SFML::Graphics SFML::Window SFML::System)

# Export symbols easily (avoids name mangling issues for C functions)
set_target_properties(plug PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    OUTPUT_NAME "plug" # Creates plug.dll
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# --------------------------------------------------------------------------------

# Find SFML (version 3.x installed via vcpkg)
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

# Create main executable
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp ${SOURCES})

# Include directories for main executable (needs both normal headers and plugin headers)
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hr_include
)

# Link SFML libraries
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)