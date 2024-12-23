@echo off
REM Change to the root directory where premake5.lua is located
cd ..

REM Define the path to premake5.exe
set PREMAKE=premake\premake5.exe

REM Check if premake5.exe exists
if not exist %PREMAKE% (
    echo Error: Premake executable not found in the "premake" folder.
    pause
    exit /b 1
)

REM Run Premake to generate project files
echo Running Premake to generate project files...
%PREMAKE% vs2022

REM Check for errors during execution
if %errorlevel% neq 0 (
    echo Error: Premake failed to generate project files.
    pause
    exit /b %errorlevel%
)

echo Premake completed successfully!
pause