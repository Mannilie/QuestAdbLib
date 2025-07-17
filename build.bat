@echo off
echo Building QuestAdbLib for Windows...

if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF ..
if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

echo.
echo Build completed successfully!
echo Examples can be found in: build\examples\Release\
echo Library files can be found in: build\Release\

pause