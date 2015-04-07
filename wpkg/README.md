ABOUT WPKG
==========

wpkg is a tool used to manage packages under MS-Windows in a way
similar to dpkg under Debian (it can actually create packages that
are 100% compatible with Debian operating systems.) It can also be
used under Linux and other Unix systems, although at this point it
is not expected to be used for a complete operating system.

This is a complete rewrite of dpkg 100% in C++. It is self-contained.
It includes a library (libdebpackages) used to read and write Debian
packages. This library can be reused by other tools to help manage
targets and build environments.

See the documentation on the Internet at
[http://windowspackager.org/](http://windowspackager.org/)
No definite documentation is included in the source at this point.
However, we are slowly documenting the library with Doxygen.

We try to distribute wpkg.exe along the source code so our users can
just download wpkg and keep going. We offer Win32 and Win64 versions.
No DLL other than the operating system DLLs are necessary.

Read INSTALL for information on how to compile wpkg under Linux
(native or mingw cross compiling,) Darwin, cygwin, or in Visual
Studio.

Note that not all Debian packages are supported because wpkg does
not support all the compression schemes that dpkg supports today.
Yet, I finally found the xz and lzma compression library so I will
add that feature at some point.


SUPPORTED OPERATING SYSTEMS / ENVIRONMENTS
==========================================

We really only support Linux Ubuntu, Windows Visual Studio, and Darwing
at this point. The following are all the systems on which wpkg was *compiled*
successfully.

* Linux

  Native Linux (64 bit, Ubuntu, Fedora)  
  MinGW cross compiling

* Windows

  Visual Studio C++ (2010 and better)  
  cygwin (latest gcc suite necessary; at least 4.x)  
  mingw (did not test compiling natively though)

* Darwin

  Mac OS/X 10.6+

* FreeBSD

  Native FreeBSD (although required compiling a newer version of g++)

* Solaris

  Native Solaris (although required intalling a newer version of g++)


KNOWN BUGS
==========

wpkg has some known bugs, some of which are inherent to the operating system
and thus cannot be fixed in wpkg itself. A complete list of the known issues
is found on the wpkg website here:

   [wpkg Known Bugs](http://windowspackager.org/documentation/wpkg/wpkg-known-bugs)


COMPILATION OF THE WPKG SOLUTION (VC++ IDE)
===========================================

When creating a solution file and running a "Build Solution", you get errors.
This is because the IDE builds everything all at once and it is incorrect in
our environment.

What you want to do is: right click on the `ALL_BUILD` target, then select
Build. That will build everything, except the packages. To then build the
packages, right click on the `wpkg_package` target, and do the same (the
`wpkg_package` target is under the package folder). However, the `wpkg_package`
only builds packages for a Release build (VC++ loads in Debug mode by
default.) Make sure that you first run the Build on the `ALL_BUILD` in the
Release configuration.

Supported configurations: We only support the Release and Debug
configurations. cmake adds other configurations that we do not use or
support in any way.


WARNING ABOUT COMPRESSION LIBRARIES
===================================

The zlib and bzip2 libraries were slightly modified to fully support
being compiled as DLL libraries.

The zconf.h checks whether `_WINDLL` is set and if so switches to the
DLL. This means if you compile zlib from any other DLL then you
pretty much automatically get to link with the zlib DLL library.
Although you still want to define `ZLIB_DLL` before including any
zlib header;

    #define ZLIB_DLL 1
    #include <zlib.h>

For this to work with one CMakeLists.txt file we added a few lines
to turn on the `ZLIB_DLL` flag when `_WINDLL` is defined. This is why
when you compile your own DLLs, you get to link against the DLL
version of the z library if you use our version.

    #  ifdef _WINDLL
    #    define ZLIB_DLL 1
    #  endif

Similarly, the bzip2 library has full DLL runtime linkage support.
The original, however, expects your library to dynamically load the
DLL and then search for the functions that you need, manually...
We added auto-linkage with the DLL import/export specification.
To make use of those, you must define `BZ_IMPORT` and `BZ_DLL` before
including the bzip2 header (`BZ_DLL` is not required if you are
creating a DLL library because you already have `_WINDLL` defined):

    #define BZ_IMPORT 1
    #define BZ_DLL 1
    #include <bzlib.h>

We changed the bzlib.h header file with the following so the library
gets compiled as a DLL that can be dynamically linked without having
to load and search for each function manually:

    #      define BZ_API(func) WINAPI func
    #      ifdef _WINDLL
    #         define BZ_EXTERN __declspec(dllexport) extern
    #      else
    #         define BZ_EXTERN extern
    #      endif
    [...]
    #      define BZ_API(func) WINAPI func
    #      if defined(BZ_DLL) || defined(_WINDLL)
    #         define BZ_EXTERN __declspec(dllimport) extern
    #      else
    #         define BZ_EXTERN extern
    #      endif

The main reason for the changes is because the CMakeLists.txt set()
command does not work on a per project basis. Instead it works on
a per directory basis. This means we cannot define the proper flags
where we need them to be. This being said, the bzip2 library did not
define dllimport/dllexport anywhere so it had to be modified.
