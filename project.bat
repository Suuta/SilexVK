@echo off

FOR /F "tokens=*" %%i IN ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property catalog_productDisplayVersion') DO (
    SET VS_VERSION=%%i
)

FOR /F "delims=." %%a IN ("%VS_VERSION%") DO (
    SET VS_MAJOR_VERSION=%%a
)

IF "%VS_MAJOR_VERSION%"=="17" (
    call Premake\premake5.exe vs2022 --file=properties.lua
) ELSE IF "%VS_MAJOR_VERSION%"=="16" (
    call Premake\premake5.exe vs2019 --file=properties.lua
) ELSE IF "%VS_MAJOR_VERSION%"=="15" (
    call Premake\premake5.exe vs2017 --file=properties.lua
) ELSE (
    echo 対応していないVisual Studioバージョンです
)

PAUSE