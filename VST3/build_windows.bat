@echo off
REM ============================================================
REM  Reverse Reverb One-Click - Windows VST3 builder
REM  Needs: Visual Studio 2022 (with "Desktop development with C++")
REM         + CMake (tick "Add to PATH" when installing, or use the
REM           CMake bundled with Visual Studio via a VS Developer Prompt)
REM  JUCE is downloaded automatically on first run (internet once).
REM ============================================================
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found on PATH.
  echo Install CMake from https://cmake.org/download/  ^(tick "Add CMake to PATH"^)
  echo ...or run this file from a "Developer Command Prompt for VS 2022".
  pause
  exit /b 1
)

echo Configuring (first run downloads JUCE, a few minutes)...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  echo [ERROR] Configure failed - is Visual Studio 2022 with C++ installed?
  pause
  exit /b 1
)

echo Building Release VST3...
cmake --build build --config Release --target ReverseReverbOneClick_VST3
if errorlevel 1 (
  echo [ERROR] Build failed - see messages above.
  pause
  exit /b 1
)

echo.
echo ============================================================
echo  DONE! Your plugin is in:
echo    build\ReverseReverbOneClick_artefacts\Release\VST3\
echo.
echo  NEXT: copy the "Reverse Reverb One-Click.vst3" folder to:
echo    C:\Program Files\Common Files\VST3\
echo  (Windows will ask for admin permission - that's normal.)
echo  Then in REAPER: Options ^> Preferences ^> Plug-ins ^> VST ^>
echo  "Re-scan" ^> "Clear cache and re-scan VST paths for all plug-ins".
echo ============================================================
echo.
choice /M "Copy it to C:\Program Files\Common Files\VST3 now"
if errorlevel 2 goto end
xcopy /E /I /Y "build\ReverseReverbOneClick_artefacts\Release\VST3\Reverse Reverb One-Click.vst3" "C:\Program Files\Common Files\VST3\Reverse Reverb One-Click.vst3"
echo Copied (if this said Access Denied, re-run this .bat as Administrator,
echo or copy the folder manually in Explorer).
:end
pause
endlocal
