//Utilities.h
#pragma once
#include <fstream>
#include <string>
#include "../Estructuras/Structs.h"

// Funciones genéricas de I/O para estructuras
template<typename T>
bool escribirStruct(std::fstream& archivo, long posicion, const T& estructura) {
    archivo.seekp(posicion);
    archivo.write(reinterpret_cast<const char*>(&estructura), sizeof(T));
    return !archivo.fail();
}

template<typename T>
bool leerStruct(std::fstream& archivo, long posicion, T& estructura) {
    archivo.seekg(posicion);
    archivo.read(reinterpret_cast<char*>(&estructura), sizeof(T));
    return !archivo.fail();
}

// Funciones de archivo
bool archivoExiste(const std::string& path);
void crearCarpetasSiNoExiste(const std::string& path);
bool crearArchivoConTamano(const std::string& path, long tamano);
bool escribirEstructuraEnArchivo(const std::string& path, const void* estructura, size_t tamano);
bool leerEstructuraDelArchivo(const std::string& path, void* estructura, size_t tamano, long posicion = 0);

// Funciones de utilidad general
std::string generarFechaActual();
int generarSignature();
void llenarConCeros(std::fstream& archivo, long bytesTotales);
std::string mayusculas(const std::string& str);
std::string cambiarExtension(const std::string& path, const std::string& nuevaExtension);
std::string obtenerDirectorioPadre(const std::string& path);
std::string formatearTiempo(time_t t);