@echo off
::
:: start and stop the RJE web service
::
if /I "%1"=="start" start "RJEWS" /D ..\rje-station cmd /c node rjews -p ..\NOS2.8.7\rjews.pid examples\rjews.json
if /I "%1"=="stop"  (
	for /F %%p in (rjews.pid) do taskkill /pid %%p /F /T
)
