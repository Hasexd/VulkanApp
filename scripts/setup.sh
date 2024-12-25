cd ..

PREMAKE="premake/premake5"

if [ ! -f "$PREMAKE" ]; then
    echo "Error: Premake executable not found in the 'premake' folder."
    exit 1
fi

cp -f premake/premake5-linux.lua premake5.lua

echo "Running Premake to generate project files..."
$PREMAKE gmake2

if [ $? -ne 0 ]; then
    echo "Error: Premake failed to generate project files."
    rm -f premake5.lua
    exit 1
fi

rm -f premake5.lua

echo "Premake completed successfully!"
