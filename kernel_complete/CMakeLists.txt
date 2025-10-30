cmake_minimum_required(VERSION 3.10)
project(KernelSimulator VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Incluir directorios
include_directories(${PROJECT_SOURCE_DIR}/include)

# Archivos fuente
set(SOURCES
    src/process.cpp
    src/scheduler.cpp
    src/memory.cpp
    src/sync.cpp
    src/main.cpp
)

# Crear ejecutable
add_executable(kernel-sim ${SOURCES})

# Flags de compilación
if(MSVC)
    target_compile_options(kernel-sim PRIVATE /W4)
else()
    target_compile_options(kernel-sim PRIVATE -Wall -Wextra -pedantic)
endif()

# Enlazar pthread para sincronización
find_package(Threads REQUIRED)
target_link_libraries(kernel-sim Threads::Threads)

# Mensaje de configuración
message(STATUS "═══════════════════════════════════════")
message(STATUS "Kernel Simulator - Configuración")
message(STATUS "═══════════════════════════════════════")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "═══════════════════════════════════════")
