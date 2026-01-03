@echo off
echo Cerrando procesos (Node y Python)...
taskkill /F /IM node.exe /T 2>nul
taskkill /F /IM python.exe /T 2>nul
echo Todo cerrado.
pause
