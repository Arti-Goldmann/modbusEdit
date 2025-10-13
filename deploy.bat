@echo off
echo Copying modbusEdit.exe to Release folder...

REM Create Release directory if it doesn't exist
if not exist "Release" mkdir Release

REM Copy the executable
copy "build\Desktop_Qt_6_9_1_MinGW_64_bit-Debug\modbusEdit.exe" "Release\modbusEdit.exe"

if %errorlevel% neq 0 (
    echo Error copying file!
    pause
    exit /b 1
)

echo File copied successfully!

REM Check if necessary Qt DLLs already exist in Release directory
echo Checking for existing Qt DLLs...
if exist "Release\Qt6Core.dll" if exist "Release\Qt6Gui.dll" if exist "Release\Qt6Widgets.dll" (
    echo Qt DLLs already exist, skipping windeployqt...
) else (
    REM Change to Release directory and run windeployqt
    echo Running windeployqt...
    cd Release
    "C:\Qt\6.9.1\mingw_64\bin\windeployqt.exe" modbusEdit.exe

    if %errorlevel% neq 0 (
        echo Error running windeployqt!
        pause
        exit /b 1
    )
    cd ..
)

echo Deployment completed successfully!
pause