@echo off
echo ======================================
echo  UniversalPointer - Restart Script
echo ======================================
echo.

REM Cerrar procesos existentes
echo [1/3] Cerrando procesos anteriores...

REM Matar Node.js en puerto 3000
for /f "tokens=5" %%a in ('netstat -aon ^| find ":3000" ^| find "LISTENING"') do (
    echo    Matando Node.js PID: %%a
    taskkill /F /PID %%a 2>nul
)

REM Matar Python ejecutando main.py
for /f "tokens=2" %%a in ('wmic process where "name='python.exe' and CommandLine like '%%main.py%%'" get ProcessId ^| findstr /r "[0-9]"') do (
    echo    Matando Python PID: %%a
    taskkill /F /PID %%a 2>nul
)

timeout /t 2 /nobreak >nul

REM Iniciar servidor Node.js
echo.
echo [2/3] Iniciando servidor Node.js...
cd sensor_server
start "UniversalPointer Server" cmd /k "node server.js"
cd ..

timeout /t 2 /nobreak >nul

REM Iniciar aplicación Python
echo.
echo [3/3] Iniciando aplicación Python...
start "UniversalPointer Main" cmd /k "python main.py"

timeout /t 2 /nobreak >nul

REM Abrir navegador
echo.
echo [OK] Abriendo navegador...
start http://localhost:3000/viewer.html

echo.
echo ======================================
echo  Aplicación iniciada correctamente
echo ======================================
echo.
pause
