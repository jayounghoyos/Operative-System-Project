@echo off
setlocal

if not exist kernel-sim.exe (
  echo [ERROR] No esta kernel-sim.exe en la raiz. Compila o renombra tu binario.
  exit /b 1
)

if not exist logs mkdir logs

for %%F in (scripts\mem_*.txt) do (
  echo === Running %%~nxF ===
  kernel-sim.exe < "%%F" > "logs\%%~nF.log"
)

for %%F in (scripts\proc_*.txt) do (
  echo === Running %%~nxF ===
  kernel-sim.exe < "%%F" > "logs\%%~nF.log"
)

echo.
echo === Checks rapidos ===
findstr /C:"PAGE FAULT" logs\*.log
findstr /C:"Evictando frame" logs\*.log
findstr /C:"QUANTUM EXPIRADO" logs\*.log
findstr /C:"TERMINADO" logs\*.log

echo Hecho.
endlocal