@echo off
echo Setting up VS 2026 Environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set CMAKE_EXE="C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set NINJA_EXE="c:/personal_projects/house_prod_suite/ninja.exe"

echo Cleaning previous build...
if exist build (rmdir /s /q build)

echo Configuring with CMake...
%CMAKE_EXE% -S . -B build -G "Ninja" -DCMAKE_MAKE_PROGRAM=%NINJA_EXE%

echo Building...
%CMAKE_EXE% --build build --config Release
