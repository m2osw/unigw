#
# CMakeLists.txt for bzip2 within the wpkg tool.
#

# WARNING: C/CXX flags are defined once for all the CMakeLists.txt projects
if ( MSVC )
    set( BZIP_FOLDER libs/bzip2 )
    # disable warnings about __int64 being larger than Int32 (warning 4244)
    # disable warnings about size_t being larger than Int32 (warning 4267)
    set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244 /wd4267 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE" )
endif ( MSVC )

set(BZIP2_INCLUDE_FILES
    bzlib.h
)

project(wpkg_bz2_static)

add_library( ${PROJECT_NAME} STATIC
    blocksort.c
    huffman.c
    crctable.c
    randtable.c
    compress.c
    decompress.c
    bzlib.c
)

StaticCompile()

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
            LINK_FLAGS ${WIN_MACHINE_FLAGS}
            FOLDER ${BZIP_FOLDER}
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
    LIBRARY DESTINATION lib COMPONENT development
    ARCHIVE DESTINATION lib COMPONENT development
)

project(wpkg_bz2)

add_library( ${PROJECT_NAME} SHARED
    blocksort.c
    huffman.c
    crctable.c
    randtable.c
    compress.c
    decompress.c
    bzlib.c
)

if( MSVC )
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS -D_WINDLL
        LINK_FLAGS ${WIN_MACHINE_FLAGS}
        FOLDER ${BZIP_FOLDER}
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
    FILES ${BZIP2_INCLUDE_FILES}
    DESTINATION include/wpkg
    COMPONENT development
)

set_property( GLOBAL APPEND PROPERTY ALL_LIBRARY_TARGETS wpkg_bz2 wpkg_bz2_static )

add_custom_target( bz2_target DEPENDS wpkg_bz2_static wpkg_bz2 )
set_property( TARGET bz2_target PROPERTY FOLDER ${BZIP_FOLDER} )


# vim: ts=4 sw=4 et
