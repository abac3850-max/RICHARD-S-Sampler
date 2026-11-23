@echo off
setlocal
cd /d "%~dp0"
echo RA Sampler - Windows x64 VST3 build
where cmake >nul 2>nul || (echo Install CMake 3.22+ & pause & exit /b 1)
where git >nul 2>nul || (echo Install Git for Windows & pause & exit /b 1)
cmake -S . -B build -A x64
if errorlevel 1 (echo CMake configure failed. Check Visual Studio 2022 C++ workload, Windows SDK and internet access. & pause & exit /b 1)
cmake --build build --config Release --target RASampler_VST3
if errorlevel 1 (echo Build failed. Save the full error output for diagnosis. & pause & exit /b 1)
set "PLUGIN="
for /d /r "build" %%D in ("RA Sampler.vst3") do set "PLUGIN=%%D"
if not defined PLUGIN (echo VST3 bundle not found. Search the build folder. & pause & exit /b 1)
set "DEST=%CommonProgramFiles%\VST3\RA Sampler.vst3"
if exist "%DEST%" rmdir /s /q "%DEST%"
xcopy "%PLUGIN%" "%DEST%" /E /I /H /Y
if errorlevel 1 (echo Copy failed. Manually copy the VST3 bundle to %CommonProgramFiles%\VST3 & pause & exit /b 1)
echo RA Sampler VST3 installed. Restart Studio One and rescan plugins.
pause
