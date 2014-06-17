# Setup toolchain so we can cross compile the mingw32 version
# directly from linux; use dev/mingw for this purpose
# Source: http://blog.beuc.net/posts/Cross-compiling_with_CMake/

# Windows references the platform as defined under
# /usr/share/cmake-2.8/Modules/Platform/
SET( CMAKE_SYSTEM_NAME Windows )
INCLUDE( CMakeForceCompiler )

# Prefix detection only works with compiler id "GNU"
# CMake will look for prefixed g++, cpp, ld, etc. automatically
CMAKE_FORCE_C_COMPILER( ${GNU_HOST}-gcc GNU )
SET( CMAKE_RC_COMPILER ${GNU_HOST}-windres )

# So other cmake scripts can detect this is MINGW32
SET( MINGW32 TRUE )
