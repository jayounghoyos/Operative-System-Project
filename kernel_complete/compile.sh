#!/bin/bash

echo "**************************"
echo "   COMPILANDO KERNEL SIMULATOR"
echo "**************************"
echo ""

# Compilar con g++
echo "[1/2] Compilando archivos fuente..."
g++ -std=c++17 -pthread -Iinclude src/*.cpp -o kernel-sim

if [ $? -eq 0 ]; then
    echo " Compilación exitosa"
    echo ""
    echo "[2/2] Ejecutable creado: ./kernel-sim"
    echo ""
    echo "**************************"
    echo "Para ejecutar: ./kernel-sim"
    echo "**************************"
else
    echo " Error en compilación"
    exit 1
fi
