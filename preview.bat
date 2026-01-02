@echo off
title Gradient Sound Preview
echo ==========================================
echo   Gradient Sound - Local Preview
echo ==========================================
echo.

:: Try to find PHP
where php >nul 2>nul
if %errorlevel% equ 0 (
    echo [+] PHP detected. Starting local server...
    echo     Opening http://localhost:8000 ...
    start http://localhost:8000
    php -S localhost:8000 -t public_html
    goto end
)

:: Try to find Python
where python >nul 2>nul
if %errorlevel% equ 0 (
    echo [+] Python detected. Starting local server...
    echo     Opening http://localhost:8000 ...
    start http://localhost:8000
    python -m http.server 8000 --directory public_html
    goto end
)

:: Fallback if no server runtime found
echo [-] No PHP or Python found.
echo [+] Opening index.html directly...
start public_html\index.html

:end
pause
