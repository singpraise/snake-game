@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"
if not exist build mkdir build

echo.
echo [1/2] Building snake DLL...

where gcc >nul 2>nul
if %ERRORLEVEL%==0 (
  echo Using gcc (MinGW-w64)...
  gcc -O2 -std=c11 -shared -DSNAKE_EXPORTS -o build\snake.dll snake.c
  if %ERRORLEVEL% neq 0 (
    echo Build failed with gcc.
    exit /b 1
  )
  echo OK: build\snake.dll
  exit /b 0
)

where cl >nul 2>nul
if %ERRORLEVEL%==0 (
  echo Using cl (Visual Studio Build Tools)...
  cl /nologo /O2 /LD snake.c /I. /Fesnake.dll
  if %ERRORLEVEL% neq 0 (
    echo Build failed with cl.
    exit /b 1
  )
  move /Y snake.dll build\snake.dll >nul
  if exist snake.lib del /q snake.lib >nul 2>nul
  if exist snake.exp del /q snake.exp >nul 2>nul
  if exist snake.obj del /q snake.obj >nul 2>nul
  echo OK: build\snake.dll
  exit /b 0
)

echo ERROR: Cannot find a C compiler.
echo - Install MinGW-w64 (gcc) or Visual Studio Build Tools (cl)
echo - Then rerun: c_core\build.bat
exit /b 1

