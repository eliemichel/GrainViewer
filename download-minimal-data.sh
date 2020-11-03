#!/bin/bash
set -e

# Download minimal additionnal data, that does not belong in a git repo
ZIPFILE=share/scenes/GrainViewer-Additional-MinimalData-v1.0.0.zip
if [ ! -f "$ZIPFILE" ]; then
	wget https://perso.telecom-paristech.fr/emichel/share/GrainViewer-MinimalData-Data-v1.0.0.zip -O $ZIPFILE
fi
unzip $ZIPFILE -d share/scenes
rm $ZIPFILE

# Check that it run all right
if [ $? -eq 0 ]
then
	echo [92mSuccessful[0m
else
	echo [91mUnsuccessful[0m
fi
