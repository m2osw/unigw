@echo off

REM Verify we're started from the right place
if not exist README (
	echo This script must be started from the top directory of the wpkg project
	exit /b 1
)
if not exist ChangeLog (
	echo This script must be started from the top directory of the wpkg project
	exit /b 1
)

REM Setup some variables
REM Unfortunately we need the version and it's not easy to extract with MS-DOS...
SET SRC="%CD%"
SET LOG="%CD%\..\wpkg-build.log"


ECHO *** wpkg builds logs under Microsoft Windows *** >%LOG%


REM
REM 64 bit
REM


REM Create build directory outside
echo ***
echo *** Prepare BUILD directory
echo ***
cd ..
if not exist BUILD goto :build_ready
	if exist BUILD\WPKG (
		rmdir /S /Q BUILD\WPKG
		timeout /T 10 /NOBREAK
	)
	rmdir /S /Q BUILD
	timeout /T 10 /NOBREAK
	REM At times the OS has a hard time to delete everything the first time...
	if exist BULID (
		rmdir /S /Q BUILD || goto :error
		timeout /T 10 /NOBREAK
	)
:build_ready
mkdir BUILD || goto :error
cd BUILD || goto :error
echo ... done


REM Run cmake in this environment
echo ***
echo *** Run cmake for Win64
echo ***
echo cmake -G "Visual Studio 10 Win64" %SRC%
cmake -G "Visual Studio 10 Win64" %SRC% || goto :error
timeout /T 3 /NOBREAK
echo ... done


REM Then compile everything in debug + release
echo Run build 64: DEBUG / ALL_BUILD
devenv.exe wpkg_project.sln /build Debug /project ALL_BUILD /log %LOG% && goto :win64_debug
devenv.exe wpkg_project.sln /build Debug /project ALL_BUILD /log %LOG% || goto :error
:win64_debug
color 07
timeout /T 25 /NOBREAK
echo Run build 64: RELEASE / ALL_BUILD
devenv.exe wpkg_project.sln /build Release /project ALL_BUILD /log %LOG% && goto :win64_release
devenv.exe wpkg_project.sln /build Release /project ALL_BUILD /log %LOG% || goto :error
:win64_release
color 07
timeout /T 25 /NOBREAK
echo Run build 64: RELEASE / wpkg_package
devenv.exe wpkg_project.sln /build Release /project wpkg_package /log %LOG% && goto :win64_packages
devenv.exe wpkg_project.sln /build Release /project wpkg_package /log %LOG% || goto :error
:win64_packages
color 07
timeout /T 25 /NOBREAK
echo ... done


REM Now we can get the version of the wpkg project
FOR /F "delims=" %%i IN ('..\BUILD\tools\Release\wpkg.exe --version') DO SET WPKG_VERSION=%%i


REM Copy result under a packages directory
echo ***
echo *** Copy results in packages\%WPKG_VERSION% directory
echo ***
cd ..
if not exist packages (
	mkdir packages || goto :error
	timeout /T 3 /NOBREAK
)
cd packages
if not exist %WPKG_VERSION% (
	mkdir %WPKG_VERSION% || goto :error
	timeout /T 3 /NOBREAK
)
cd %WPKG_VERSION%
copy ..\..\BUILD\tools\Release\wpkg.exe wpkg64_%WPKG_VERSION%.exe || goto :error
copy ..\..\BUILD\WPKG\packages\*-amd64.deb . || goto :error
cd ..\..


REM
REM 32 bit
REM


REM Create the BUILD32 for i386 version
echo ***
echo *** Prepare BUILD32 directory
echo ***
if not exist BUILD32 goto :build32_ready
	if exist BUILD32\WPKG (
		rmdir /S /Q BUILD32\WPKG
		timeout /T 10 /NOBREAK
	)
	rmdir /S /Q BUILD32
	timeout /T 10 /NOBREAK
	REM At times the OS has a hard time to delete everything the first time...
	if exist BULID32 (
		rmdir /S /Q BUILD32 || goto :error
		timeout /T 10 /NOBREAK
	)
:build32_ready
mkdir BUILD32 || goto :error
cd BUILD32 || goto :error
echo ... done

REM Run cmake in this environment
echo ***
echo *** Run cmake for Win32
echo ***
echo cmake -G "Visual Studio 10" %SRC%
cmake -G "Visual Studio 10" %SRC% || goto :error
timeout /T 3 /NOBREAK
echo ... done


REM Then compile everything in debug + release
echo Run build 32: DEBUG / ALL_BUILD
devenv.exe wpkg_project.sln /build Debug /project ALL_BUILD /log %LOG% && goto :win32_debug
devenv.exe wpkg_project.sln /build Debug /project ALL_BUILD /log %LOG% || goto :error
:win32_debug
color 07
timeout /T 25 /NOBREAK
echo Run build 32: RELEASE / ALL_BUILD
devenv.exe wpkg_project.sln /build Release /project ALL_BUILD /log %LOG% && goto :win32_release
devenv.exe wpkg_project.sln /build Release /project ALL_BUILD /log %LOG% || goto :error
:win32_release
color 07
timeout /T 25 /NOBREAK
echo Run build 32: RELEASE / wpkg_package
devenv.exe wpkg_project.sln /build Release /project wpkg_package /log %LOG% && goto :win32_packages
devenv.exe wpkg_project.sln /build Release /project wpkg_package /log %LOG% || goto :error
:win32_packages
color 07
timeout /T 25 /NOBREAK
echo ... done


REM Copy result under a packages directory
echo ***
echo *** Copy results in packages\%WPKG_VERSION% directory
echo ***
cd ..\packages\%WPKG_VERSION%
copy ..\..\BUILD32\tools\Release\wpkg.exe wpkg_%WPKG_VERSION%.exe || goto :error
copy ..\..\BUILD32\WPKG\packages\*-i386.deb . || goto :error


echo ... done
goto :out

:error
color 07
echo +++
echo +++ AN ERROR OCCURED!
echo +++

:out
REM Restore directory (I tried with pushd + popd and it did not work...)
cd %SRC%
