cmake_minimum_required(VERSION 3.30)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# The version number here is the canonical one for the project.
# Run rs-bump-version to increment the patch number
# and propagate the new version to version.hpp.

project(rs-graphics
    VERSION 0.1.2
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
    add_compile_options(/EHsc /Gy /MP /O2 /sdl /utf-8 /W4 /WX)
else()
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    add_compile_options(-finput-charset=UTF-8 -march=native -O2
        -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wsuggest-override -Wsuggest-destructor-override)
    endif()
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX=1 UNICODE=1 _UNICODE=1 WINVER=0xa00 _WIN32_WINNT=0xa00)
else()
    add_compile_definitions(_REENTRANT=1 _XOPEN_SOURCE=700)
    if(APPLE)
        add_compile_definitions(_DARWIN_C_SOURCE=1)
    endif()
endif()

find_package(Threads REQUIRED)
include_directories(.)
set(library rs-graphics)
set(unittest test-rs-graphics)

set(linklibs
    PRIVATE ${library}
    PRIVATE rs-unicode
    PRIVATE pcre2-8
    PRIVATE Threads::Threads
)

add_library(${library} STATIC
    rs-graphics/font.cpp
    rs-graphics/image.cpp
)

add_executable(${unittest}
    test/colour-space-test.cpp
    test/colour-floating-channel-test.cpp
    test/colour-integer-channel-test.cpp
    test/colour-alpha-test.cpp
    test/colour-conversion-test.cpp
    test/colour-interpolation-test.cpp
    test/colour-string-test.cpp
    test/font-test.cpp
    test/geometry-test.cpp
    test/image-test.cpp
    test/noise-test.cpp
    test/root-finding-test.cpp
    test/projection-test.cpp
    test/version-test.cpp
    test/unit-test.cpp
)

target_link_libraries(${unittest} ${linklibs})

install(DIRECTORY ${library} DESTINATION include PATTERN "*.cpp" EXCLUDE)
install(TARGETS ${library} LIBRARY DESTINATION lib)
