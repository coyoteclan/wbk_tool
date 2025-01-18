cmake_minimum_required(VERSION 3.10)
project(c1cx LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MinGW cross-compilation setup
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Static linking flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -mwindows")

add_executable(wbk_tool
    wbk_tool.cpp
    ima_adpcm.h
    wav.h
    wbk.h
    wbk_tool.res.o
)

target_link_options(wbk_tool PRIVATE -s -Wl,--enable-stdcall-fixup)

target_compile_definitions(wbk_tool PRIVATE WIN32 _WINDOWS)

target_link_libraries(wbk_tool PRIVATE uxtheme)
