#!/bin/bash
set -e

echo "Limpiando build..."
rm -rf build

echo "Creando carpeta build..."
mkdir build

echo "Entrando a build..."
cd build

echo "Ejecutando CMake..."
cmake ..

echo "Compilando proyecto..."
make

echo "Regresando al directorio principal..."
cd ..

printf "Compilando...\nEjecutando programa...\n"
echo " "
./build/backend