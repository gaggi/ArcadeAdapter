@echo off
cd /d "%~dp0"  
bootloadHID.exe -r ArcadeAdapter_0.1.hex
pause