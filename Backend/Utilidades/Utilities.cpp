//Utilities.cpp
#include "Utilities.h"
#include <filesystem>
#include <ctime>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace fs = std::filesystem;

// (Implementations moved to Utilities.h)

//FUNCIONES DE ARCHIVO

//Verificar si un archivo existe
bool archivoExiste(const std::string& path) {
    return fs::exists(path);
}

//Crear directorios si no existen
void crearCarpetasSiNoExiste(const std::string& path) {
    try {
        std::string parentDir = obtenerDirectorioPadre(path);
        if (!parentDir.empty() && parentDir != ".") {
            fs::create_directories(parentDir);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creando directorios: " << e.what() << std::endl;
    }
}

//Crear un archivo con un tamaño específico, llenado con ceros
bool crearArchivoConTamano(const std::string& path, long tamano) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "Error: No se pudo crear el archivo: " << path << std::endl;
            return false;
        }

        //Llenar el archivo con ceros usando un buffer de 1024 bytes
        char buffer[1024] = {0};
        long remaining = tamano;

        while (remaining > 0) {
            long writeSize = remaining > 1024 ? 1024 : remaining;
            file.write(buffer, writeSize);
            if (!file) {
                std::cerr << "Error: No se pudo escribir en el archivo" << std::endl;
                file.close();
                return false;
            }
            remaining -= writeSize;
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creando archivo con tamaño: " << e.what() << std::endl;
        return false;
    }
}

//Escribir una estructura en el inicio de un archivo
bool escribirEstructuraEnArchivo(const std::string& path, const void* estructura, size_t tamano) {
    try {
        std::ofstream file(path, std::ios::binary | std::ios::in);
        if (!file) {
            std::cerr << "Error: No se pudo abrir el archivo: " << path << std::endl;
            return false;
        }

        file.write(reinterpret_cast<const char*>(estructura), tamano);
        if (!file) {
            std::cerr << "Error: No se pudo escribir en el archivo" << std::endl;
            file.close();
            return false;
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error escribiendo estructura: " << e.what() << std::endl;
        return false;
    }
}

//Leer una estructura de un archivo desde una posición
bool leerEstructuraDelArchivo(const std::string& path, void* estructura, size_t tamano, long posicion) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "Error: No se pudo abrir el archivo: " << path << std::endl;
            return false;
        }

        file.seekg(posicion);
        file.read(reinterpret_cast<char*>(estructura), tamano);
        if (!file) {
            std::cerr << "Error: No se pudo leer del archivo" << std::endl;
            file.close();
            return false;
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error leyendo estructura: " << e.what() << std::endl;
        return false;
    }
}

//FUNCIONES DE UTILIDAD

//Generar la fecha y hora actual como string
std::string generarFechaActual() {
    time_t now = std::time(nullptr);
    char buffer[80];
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

//Generar un número aleatorio para signature
int generarSignature() {
    return rand();
}

// Llenar un archivo con ceros a partir de una posición
void llenarConCeros(std::fstream& archivo, long bytesTotales) {
    char buffer[1024] = {0};
    long remaining = bytesTotales;

    while (remaining > 0) {
        long writeSize = remaining > 1024 ? 1024 : remaining;
        archivo.write(buffer, writeSize);
        remaining -= writeSize;
    }
}

//Convertir string a mayúsculas
std::string mayusculas(const std::string& str) {
    std::string result = str;
    for (auto& c : result) {
        c = std::toupper(c);
    }
    return result;
}

//Cambiar la extensión de un archivo
std::string cambiarExtension(const std::string& path, const std::string& nuevaExtension) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        return path.substr(0, dotPos) + (nuevaExtension[0] == '.' ? nuevaExtension : "." + nuevaExtension);
    }
    return path + (nuevaExtension[0] == '.' ? nuevaExtension : "." + nuevaExtension);
}

//Obtener el directorio padre de una ruta
std::string obtenerDirectorioPadre(const std::string& path) {
    fs::path p(path);
    return p.parent_path().string();
}

//Formatear un tiempo (time_t) a string
std::string formatearTiempo(time_t t) {
    if (t == 0) return "N/A";
    char buffer[80];
    struct tm* timeinfo = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}