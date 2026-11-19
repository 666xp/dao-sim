@echo off
set ROOT=%~dp0
if not exist "%ROOT%sim\build\sim_engine.exe" if not exist "%ROOT%sim\build\sim_engine" (
  echo Build the engine first: cmake -S sim -B sim\build ^&^& cmake --build sim\build -j
  exit /b 1
)
start "dao-sim-engine" "%ROOT%sim\build\sim_engine.exe"
start "dao-sim-web" cmd /c "npm --prefix \"%ROOT%web\" run dev"
echo engine ws://127.0.0.1:3848  web http://127.0.0.1:3847
echo Close the engine/web windows to stop.
