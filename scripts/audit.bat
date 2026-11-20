@echo off
cd /d "%~dp0.."
python scripts\audit.py
exit /b %ERRORLEVEL%
