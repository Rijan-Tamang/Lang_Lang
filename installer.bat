@echo off
echo Building Lang_Lang Compiler...
echo.

g++ *.cpp -std=c++17 -o llc.exe

echo.
if %errorlevel% neq 0 (
    echo ==========================
    echo BUILD FAILED
    echo ==========================
) else (
    echo ==========================
    echo BUILD SUCCESSFUL
    echo ==========================
)

pause