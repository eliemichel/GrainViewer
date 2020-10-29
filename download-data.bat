:: Download additionnal heavy data, that does not belong in a git repo
powershell -command "Start-BitsTransfer -Source https://perso.telecom-paristech.fr/emichel/share/GrainViewer-Additional-Data-v1.0.0.zip -Destination share/scenes/GrainViewer-Additional-Data-v1.0.0.zip"
powershell -command "Expand-Archive share/scenes/GrainViewer-Additional-Data-v1.0.0.zip share/scenes"

@echo off
:: Check that it run all right
if errorlevel 1 (
	echo [91mUnsuccessful[0m
) else (
	echo [92mSuccessful[0m
)
pause
