@echo off
echo Cerrando procesos específicos del proyecto...

REM Matar solo Node.js del sensor_server (por puerto 3000)
for /f "tokens=5" %%a in ('netstat -aon ^| find ":3000" ^| find "LISTENING"') do taskkill /F /PID %%a 2>nul

REM Matar solo Python que ejecuta main.py (buscar por línea de comando)
for /f "tokens=2" %%a in ('wmic process where "name='python.exe' and CommandLine like '%%main.py%%'" get ProcessId ^| findstr /r "[0-9]"') do taskkill /F /PID %%a 2>nul

echo Procesos del proyecto cerrados (Claude Code sigue funcionando).
pause
