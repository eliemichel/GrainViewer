:: Download minimal additionnal data, that does not belong in a git repo
set ZIPFILE=share\scenes\GrainViewer-Additional-MinimalData-v1.0.0.zip
powershell -command "Start-BitsTransfer -Source https://perso.telecom-paristech.fr/emichel/share/GrainViewer-Additional-MinimalData-v1.0.0.zip -Destination %ZIPFILE%"
powershell -command "Expand-Archive %ZIPFILE% share\scenes"
del %ZIPFILE%

@echo off
:: Check that it run all right
if errorlevel 1 (
	echo [91mUnsuccessful[0m
) else (
	echo [92mSuccessful[0m
)
pause
