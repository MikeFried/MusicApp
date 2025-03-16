cmake_minimum_required(VERSION 3.15)
project(MusicApp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    MusicApp.cpp
    DeviceManager.cpp
    ConfigDialog.cpp
)

# Add header files
set(HEADERS
    MusicApp.h
    DeviceManager.h
    ConfigDialog.h
)

# Add resource files
set(RESOURCES
    ConfigDialog.rc
)

# Create executable
add_executable(MusicApp WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# Link required libraries
target_link_libraries(MusicApp PRIVATE
    winmm
    comctl32
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directory for resources
target_include_directories(MusicApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) 