#
# CMakeLists.txt for libz within the wpkg tool.
#

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if ( MSVC )
    set( ZLIB_FOLDER libs/zlib )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE" )
endif()

# TODO: make zconf.h from zconf.h.cmakein
set(ZLIB_PUBLIC_HDRS
    zconf.h
    zlib.h
)

project(wpkg_z_static)

add_library( ${PROJECT_NAME} STATIC
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        FOLDER ${ZLIB_FOLDER}
    )
endif()

StaticCompile()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)



project(wpkg_z)

add_library( ${PROJECT_NAME} SHARED
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS -DZLIB_DLL
        FOLDER ${ZLIB_FOLDER}
    )
else()
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VERSION ${WPKG_VERSION_MAJOR}.${WPKG_VERSION_MINOR}
        SOVERSION ${WPKG_VERSION_MAJOR}
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT development
)
install(
    FILES ${ZLIB_PUBLIC_HDRS}
    DESTINATION include/wpkg
    COMPONENT headers
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS wpkg_z wpkg_z_static )

add_custom_target( zlib_target DEPENDS wpkg_z_static wpkg_z )
set_property( TARGET zlib_target PROPERTY FOLDER ${ZLIB_FOLDER} )

# vim: ts=4 sw=4 et
