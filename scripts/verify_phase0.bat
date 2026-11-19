@echo off
REM Phase 0 only: graph / event_queue / neuron / validate.
REM Does NOT build engine or phase-2 dynamics tests.
setlocal
cd /d "%~dp0.."
set BUILD=sim\build-phase0

cmake -S sim -B %BUILD% -DDAO_SIM_BUILD_ENGINE=OFF -DDAO_SIM_BUILD_DYNAMICS=OFF -DDAO_SIM_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

cmake --build %BUILD% -j
if errorlevel 1 exit /b 1

ctest --test-dir %BUILD% --output-on-failure
if errorlevel 1 exit /b 1

if exist %BUILD%\Release\sim_validate.exe (
  %BUILD%\Release\sim_validate.exe 42
) else if exist %BUILD%\sim_validate.exe (
  %BUILD%\sim_validate.exe 42
) else (
  %BUILD%\Debug\sim_validate.exe 42
)
if errorlevel 1 exit /b 1

echo PASS: phase0_verify_complete 1
endlocal
