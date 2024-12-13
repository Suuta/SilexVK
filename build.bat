@echo off

FOR /F "tokens=*" %%i IN ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') DO (
    SET VSPATH=%%i\MSBuild\Current\Bin
)

"%VSPATH%\MSBuild.exe" "Silex.sln" /p:Configuration=Release /p:Platform="x64" -m

PAUSE
