cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find fmt package
find_package(fmt CONFIG REQUIRED)

# Find FFmpeg packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)
find_package(hwy CONFIG REQUIRED)
add_executable(highway-it highway-it.cpp highway-it-main.cpp)
target_link_libraries(highway-it PRIVATE hwy::hwy)

add_executable(HelloWorld main.cpp)
add_executable(av-it av-it2.cpp)

# Link fmt library to HelloWorld
target_link_libraries(HelloWorld PRIVATE fmt::fmt)

# Link fmt and FFmpeg libraries to av-it
target_link_libraries(av-it PRIVATE 
    fmt::fmt
    PkgConfig::LIBAV
)

add_executable(bullet bullet.cpp)
find_package(Bullet CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
target_link_libraries(bullet PRIVATE raylib ${BULLET_LIBRARIES})

add_executable(tray-app WIN32 tray-app.cpp)

