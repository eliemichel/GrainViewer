#!/bin/bash
# Download additionnal heavy data, that does not belong in a git repo
wget https://perso.telecom-paristech.fr/emichel/share/GrainViewer-Additional-Data-v1.0.0.zip -O share/scenes/GrainViewer-Additional-Data-v1.0.0.zip
unzip share/scenes/GrainViewer-Additional-Data-v1.0.0.zip -d share/scenes

# Check that it run all right
if [ $? -eq 0 ]
then
	echo [92mSuccessful[0m
else
	echo [91mUnsuccessful[0m
fi
