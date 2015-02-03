
  INTRODUCTION
================

This sample is an attempt in documenting the full process of converting
a project from source to a set of wpkg packages that work in an HTTP
repository.

You can see this project as a supplemental documention from the various
pages available on the project website.

If you want to go straight to the neat stuff, look at the build scripts
that match your computer (i.e. build.linux). Those scripts are very
detailed. You are not really expected to write such a script, this is
done to show you all the steps I have to take to get a repository to
work on my system and you should be able to replicate most of that on
your own system. A few things require root access to setup Apache2.
This is also documented appropriately directly in the script.


  SOURCE
==========

  Pick a Project
------------------

First get the source of a project. Here I chose zlib which is
fairly easy to deal with and already part of the wpkg project.

I have a specific copy of the zlib project in this sample so
it can include all the files as to install a full version of
zlib and not just a zlib version along wpkg (if you look
closely, the wpkg version of zlib is named libwpkg_z.so/dll
instead of the normal libz.so/dll because we use a specific
version just and only for wpkg to avoid upgrade problems
under MS-Windows.)


  CMake Compatible
--------------------

You can find the original source of the zlib project in the contrib
directory.

The default CMakeLists.txt *works*, but not with the wpkg environment
which expects files to be installed in the correct sub-directory. This
is done using the COMPONENT option of the INSTALL macro.

  http://www.cmake.org/Wiki/CMake:Component_Install_With_CPack

You would end up with INSTALL macros as such:

  INSTALL(TARGETS SimpleInstall test1 test2 test3
    RUNTIME DESTINATION MyTest/bin        COMPONENT runtime       # .exe, .dll
    LIBRARY DESTINATION MyTest/lib        COMPONENT runtime       # .so, mod.dll
    ARCHIVE DESTINATION MyTest/lib/static COMPONENT development   # .a, .lib
  )
  INSTALL( # .man, .html
      DIRECTORY ${PROJECT_SOURCE_DIR}/doc
      DESTINATION share/doc/MyTest
      COMPONENT documentation
      FILES_MATCHING PATTERN *.html
  )

The project here also includes a WPKG directory with the necessary
control file and some optional files like the copyright file.

Unfortunately, the default CMakeLists.txt file from zlib is extremely
complicated so I do not use that at all. I just create my own version
which, in comparison, looks dead simple.


  wpkg compatible
-------------------

The wpkg environment, when creating a source package, is checked for a
"large" number of files and their validity.

This includes:

  * README

    The project must include a README file. One of the following will
    do to satisfy this requirement:

    project-directory/README
    project-directory/README.txt

  * INSTALL

    The project is expected to include an INSTALL file. One of the
    following will do to satisfy this requirement:

    project-directory/INSTALL
    project-directory/INSTALL.txt

  * ChangeLog

    The project must have a changelog file. One of the following
    will do to satisfy this requirement:

    project-directory/wpkg/changelog  (preferred)
    project-directory/debian/changelog
    project-directory/changelog
    project-directory/Changelog
    project-directory/ChangeLog

    The format of the changelog file is strongly checked. If it does
    not match the Debian changelog format, then the file is rejected
    with an error and the source not created.

    In case of the zlib library, there is a ChangeLog file which is
    not compatible at all by default. We have to either move it out
    of the way (give it a different name) or edit it to be compatible
    (which in general is preferable).




vim: ts=4 sw=4 et
