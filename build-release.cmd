@echo off
setlocal
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "CELL_VS=%%I"
if not defined CELL_VS exit /b 1
call "%CELL_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
cmake -S "%~dp0." -B "%~dp0build\Release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF %*
if errorlevel 1 exit /b 1
cmake --build "%~dp0build\Release" --parallel 4
exit /b %errorlevel%
