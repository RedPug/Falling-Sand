@echo off
setlocal enabledelayedexpansion

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Find all .cpp files and build the list
set "sources="
for /r src %%f in (*.cpp) do (
    set "sources=!sources! %%f"
)

echo Found source files: %sources%

REM Compile with all found source files
gcc %sources% -o ./build/main.exe -I "./src" -I "C:\\SDL\\x86_64-w64-mingw32\\include" -L "C:\\SDL\\x86_64-w64-mingw32\\lib" -L "C:\\SDL\\x86_64-w64-mingw32\\bin" -lstdc++ -lSDL3

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
) else (
    echo Build failed!
    pause
)
