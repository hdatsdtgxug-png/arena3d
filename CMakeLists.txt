cmake_minimum_required(VERSION 3.15)
project(Arena3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
set(FETCHCONTENT_QUIET FALSE)
FetchContent_MakeAvailable(raylib)

add_executable(Arena3D main.cpp)
target_link_libraries(Arena3D raylib)

if (WIN32)
    target_link_libraries(Arena3D winmm)
    target_compile_definitions(Arena3D PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601)
    if (MINGW)
        target_link_options(Arena3D PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()
