#!/bin/bash
set -e

echo "Matando instancias anteriores del backend (si existen)..."
pkill -f "./build/backend" 2>/dev/null || true
fuser -k 8080/tcp 2>/dev/null || true
sleep 1

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