@echo off
setlocal

clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX src\main.cpp -o pyright-langserver.exe
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build succeeded: pyright-langserver.exe
