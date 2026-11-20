@echo off
setlocal
set ROOT=%~dp0

if exist "%ROOT%sim\build\Debug\sim_engine.exe" (
  set ENGINE=%ROOT%sim\build\Debug\sim_engine.exe
) else if exist "%ROOT%sim\build\Release\sim_engine.exe" (
  set ENGINE=%ROOT%sim\build\Release\sim_engine.exe
) else if exist "%ROOT%sim\build\sim_engine.exe" (
  set ENGINE=%ROOT%sim\build\sim_engine.exe
) else (
  echo sim_engine.exe not found. Build first:
  echo   cmake -S sim -B sim\build -DDAO_SIM_BUILD_ENGINE=ON -DDAO_SIM_BUILD_DYNAMICS=ON
  echo   cmake --build sim\build --config Debug
  exit /b 1
)

echo Starting engine: %ENGINE% --fake
start "dao-sim-engine" "%ENGINE%" --fake --port 3848
start "dao-sim-web" cmd /c "npm --prefix \"%ROOT%web\" run dev"
echo engine ws://127.0.0.1:3848  web http://127.0.0.1:3847
echo Phase 1: fake telemetry. Close the engine/web windows to stop.
echo Cloud smoke != acceptance; confirm curves in a local browser.
endlocal
