@echo off
echo Iniciando Servidor Bridge (Node.js)...
start /B cmd /c "cd sensor_server && node server.js"
timeout /t 2 > nul
echo Iniciando Aplicacion Python...
rem start "" "http://localhost:3000/viewer.html"
python main.py
pause
