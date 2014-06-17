# Found this in the ArxBuild project
# https://github.com/arx/ArxBuild/blob/master/toolchains/gcc-4.8-toolchain.cmake
# In that project they actually check all versions, we just check the latest
# as of today (4.8.0 20121014)
find_program(GCC_48 NAMES
    gcc-4.8
    gcc48
    gcc-4.8.9
    gcc-4.8.8
    gcc-4.8.7
    gcc-4.8.6
    gcc-4.8.5
    gcc-4.8.4
    gcc-4.8.3
    gcc-4.8.2
    gcc-4.8.1
    gcc-4.8.0
    gcc-4.8.0-pre9999
)
mark_as_advanced(GCC_48)
find_program(GXX_48 NAMES
    g++-4.8
    g++48
    g++-4.8.9
    g++-4.8.8
    g++-4.8.7
    g++-4.8.6
    g++-4.8.5
    g++-4.8.4
    g++-4.8.3
    g++-4.8.2
    g++-4.8.1
    g++-4.8.0
    g++-4.8.0-pre9999
)
mark_as_advanced(GXX_48)

if((NOT GCC_48) OR (NOT GXX_48))
    message(FATAL_ERROR "GCC 4.8 not found (found gcc=\"${GCC_48}\", g++=\"${GXX_48}\")")
endif()

# which compilers to use for C and C++
set(CMAKE_C_COMPILER "${GCC_48}")
set(CMAKE_CXX_COMPILER "${GXX_48}")

# vim: ts=4 sw=4 et
