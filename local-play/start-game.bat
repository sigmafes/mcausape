@echo off
REM Inicia el servidor local de MCPE 0.6.1 usando el Node incluido en Qoder.
cd /d "%~dp0"
set ELECTRON_RUN_AS_NODE=1
echo Servidor en http://localhost:8137/  (Ctrl+C para parar)
"%LOCALAPPDATA%\Programs\Qoder\Qoder.exe" serve.js
