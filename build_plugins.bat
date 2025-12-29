@echo off
set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
set "CMAKE_PATH=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

echo Building...
"%CMAKE_PATH%" --build build --config Release
if %errorlevel% neq 0 exit /b %errorlevel%

echo Build Done.
