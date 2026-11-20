@echo off
cd /d "%~dp0.."
python scripts\collect_lights.py --out web\public\lights.json
if errorlevel 1 exit /b 1
python scripts\collect_lights.py --inject-fail --out web\public\lights.fail.json
echo Done. Open http://127.0.0.1:3847/lights
exit /b 0
