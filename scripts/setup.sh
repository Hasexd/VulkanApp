#!/usr/bin/bash

echo "Installing required packages..."
sudo apt-get update
sudo apt-get install -y libvulkan-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev


cd ..

PREMAKE="premake/premake5"

if [ ! -x "$PREMAKE" ]; then
	echo "Error: Premake executable not found in the 'premake' folder"
	exit 1
fi

cp -f premake/premake5-linux.lua premake5.lua

echo "Running premake to generate project files"
$PREMAKE gmake2

rm -f premake5.lua
echo "Premake completed succesfully!"

