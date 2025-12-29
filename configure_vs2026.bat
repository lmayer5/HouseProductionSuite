@echo off
set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
set "CMAKE_PATH=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

echo Configuring with CMake (Ninja)...
"%CMAKE_PATH%" -B build -G "Ninja" -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%"
if %errorlevel% neq 0 exit /b %errorlevel%

echo Configuration Done.
