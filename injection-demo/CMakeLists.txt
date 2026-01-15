cmake_minimum_required(VERSION 3.10)
project(InjectionDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Target application with window to protect
add_executable(target-app WIN32 target-app.cpp)
target_link_libraries(target-app PRIVATE user32 gdi32)

# Hook DLL that intercepts screenshot APIs
add_library(hook-dll SHARED hook-dll.cpp)
target_link_libraries(hook-dll PRIVATE user32 gdi32)

# Injector that performs DLL injection
add_executable(injector injector.cpp)
target_link_libraries(injector PRIVATE user32)
