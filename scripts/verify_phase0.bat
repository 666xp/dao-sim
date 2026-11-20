@echo off
REM Phase 0 only: graph / event_queue / neuron / validate.
REM Does NOT build engine or phase-2 dynamics tests.
REM ctest and sim_validate are recorded separately, then summarized.
setlocal EnableDelayedExpansion
cd /d "%~dp0.."
set BUILD=sim\build-phase0
set CTEST_RC=0
set VALIDATE_RC=0

cmake -S sim -B %BUILD% -DDAO_SIM_BUILD_ENGINE=OFF -DDAO_SIM_BUILD_DYNAMICS=OFF -DDAO_SIM_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

cmake --build %BUILD% --config Debug -j
if errorlevel 1 exit /b 1

ctest --test-dir %BUILD% -C Debug --output-on-failure
set CTEST_RC=!errorlevel!

set VALIDATE=
if exist %BUILD%\Debug\sim_validate.exe (
  set VALIDATE=%BUILD%\Debug\sim_validate.exe
) else if exist %BUILD%\Release\sim_validate.exe (
  set VALIDATE=%BUILD%\Release\sim_validate.exe
) else if exist %BUILD%\sim_validate.exe (
  set VALIDATE=%BUILD%\sim_validate.exe
)

if defined VALIDATE (
  echo Running: !VALIDATE! 42
  "!VALIDATE!" 42
  set VALIDATE_RC=!errorlevel!
) else (
  echo ERROR: sim_validate.exe not found under %BUILD%
  set VALIDATE_RC=1
)

echo.
echo --- phase0 summary ---
echo ctest_exit=!CTEST_RC!  validate_exit=!VALIDATE_RC!
if not "!CTEST_RC!"=="0" (
  echo FAIL: ctest
  exit /b 1
)
if not "!VALIDATE_RC!"=="0" (
  echo FAIL: sim_validate
  exit /b 1
)

echo PASS: phase0_verify_complete 1
endlocal
exit /b 0
